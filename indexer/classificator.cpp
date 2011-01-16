#include "../base/SRC_FIRST.hpp"
#include "classificator.hpp"
#include "tree_structure.hpp"

#include "../coding/file_reader.hpp"

#include "../base/assert.hpp"

#include "../std/target_os.hpp"
#include "../std/bind.hpp"
#include "../std/algorithm.hpp"
#include "../std/iterator.hpp"

#include "../base/start_mem_debug.hpp"


/////////////////////////////////////////////////////////////////////////////////////////
// ClassifObject implementation
/////////////////////////////////////////////////////////////////////////////////////////

ClassifObject * ClassifObject::AddImpl(string const & s)
{
  if (m_objs.empty()) m_objs.reserve(30);

  m_objs.push_back(ClassifObject(s));
  return &(m_objs.back());
}

ClassifObject * ClassifObject::Add(string const & s)
{
  ClassifObject * p = Find(s);
  return (p ? p : AddImpl(s));
}

void ClassifObject::AddCriterion(string const & s)
{
  Add('[' + s + ']');
}

ClassifObject * ClassifObject::Find(string const & s)
{
  for (iter_t i = m_objs.begin(); i != m_objs.end(); ++i)
    if ((*i).m_name == s)
      return &(*i);

  return 0;
}

void ClassifObject::AddDrawRule(drule::Key const & k)
{
  for (size_t i = 0; i < m_drawRule.size(); ++i)
    if (k == m_drawRule[i]) return;

  m_drawRule.push_back(k);
}

bool ClassifObject::IsCriterion() const
{
  return (m_name[0] == '[');
}

ClassifObjectPtr ClassifObject::BinaryFind(string const & s) const
{
  const_iter_t i = lower_bound(m_objs.begin(), m_objs.end(), s, less_name_t());
  if ((i == m_objs.end()) || ((*i).m_name != s))
    return ClassifObjectPtr(0, 0);
  else
    return ClassifObjectPtr(&(*i), distance(m_objs.begin(), i));
}

void ClassifObject::SavePolicy::Serialize(ostream & s) const
{
  ClassifObject const * p = Current();
  for (size_t i = 0; i < p->m_drawRule.size(); ++i)
    s << p->m_drawRule[i].toString() << "  ";
}

void ClassifObject::LoadPolicy::Serialize(string const & s)
{
  ClassifObject * p = Current();

  // load drawing rule
  drule::Key key;
  key.fromString(s);
  p->m_drawRule.push_back(key);

  // mark as visible in rule's scale
  p->m_visibility[key.m_scale] = true;
}

void ClassifObject::LoadPolicy::Start(size_t i)
{
  ClassifObject * p = Current();
  p->m_objs.push_back(ClassifObject());

  base_type::Start(i);
}

void ClassifObject::LoadPolicy::EndChilds()
{
  ClassifObject * p = Current();
  ASSERT ( p->m_objs.back().m_name.empty(), () );
  p->m_objs.pop_back();
}

void ClassifObject::VisSavePolicy::Serialize(ostream & s) const
{
  ClassifObject const * p = Current();

  size_t const count = p->m_visibility.size();

  string str;
  str.resize(count);
  for (size_t i = 0; i < count; ++i)
    str[i] = p->m_visibility[i] ? '1' : '0';

  s << str << "  ";
}

void ClassifObject::VisLoadPolicy::Name(string const & name) const
{
  static_cast<void>(name);
  // Assume that classificator doesn't changed for saved visibility.
  ASSERT ( name == Current()->m_name, () );
}

void ClassifObject::VisLoadPolicy::Serialize(string const & s)
{
  ClassifObject * p = Current();

  for (size_t i = 0; i < s.size(); ++i)
    p->m_visibility[i] = (s[i] == '1');
}

void ClassifObject::VisLoadPolicy::Start(size_t i)
{
  if (i < Current()->m_objs.size())
    base_type::Start(i);
  else
    m_stack.push_back(0); // dummy
}

void ClassifObject::Sort()
{
  sort(m_objs.begin(), m_objs.end(), less_name_t());
  for_each(m_objs.begin(), m_objs.end(), boost::bind(&ClassifObject::Sort, _1));
}

void ClassifObject::Swap(ClassifObject & r)
{
  swap(m_name, r.m_name);
  swap(m_drawRule, r.m_drawRule);
  swap(m_objs, r.m_objs);
  swap(m_visibility, r.m_visibility);
}

ClassifObject const * ClassifObject::GetObject(size_t i) const
{
  ASSERT ( i < m_objs.size(), (i) );
  return &(m_objs[i]);
}

void ClassifObject::ConcatChildNames(string & s) const
{
  s.clear();
  size_t const count = m_objs.size();
  for (size_t i = 0; i < count; ++i)
  {
    s += m_objs[i].GetName();
    if (i != count-1) s += '|';
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
// Classificator implementation
/////////////////////////////////////////////////////////////////////////////////////////

Classificator & classif()
{
  static Classificator c;
  return c;
}

namespace ftype
{
  uint8_t const bits_count = 6;
  uint8_t const levels_count = 5;
  uint8_t const max_value = (1 << bits_count) - 1;

  void set_value(uint32_t & type, uint8_t level, uint8_t value)
  {
    level *= bits_count;  // level to bits

    uint32_t const m1 = uint32_t(max_value) << level;
    type &= (~m1);        // zero bits

    uint32_t const m2 = uint32_t(value) << level;
    type |= m2;           // set bits
  }

  uint8_t get_value(uint32_t type, uint8_t level)
  {
    level *= bits_count;     // level to bits;

    uint32_t const m = uint32_t(max_value) << level;
    type &= m;              // leave only our bits

    type = type >> level;   // move to start
    ASSERT ( type <= max_value, ("invalid output value", type) );

    return uint8_t(type);   // conversion
  }

  uint8_t get_control_level(uint32_t type)
  {
    uint8_t count = 0;
    while (type > 1)
    {
      type = type >> bits_count;
      ++count;
    }

    return count;
  }

  void PushValue(uint32_t & type, uint8_t value)
  {
    ASSERT ( value <= max_value, ("invalid input value", value) );

    uint8_t const cl = get_control_level(type);
    ASSERT ( cl < levels_count, (cl) );

    set_value(type, cl, value);

    // set control level
    set_value(type, cl+1, 1);
  }

  bool GetValue(uint32_t type, uint8_t level, uint8_t & value)
  {
    ASSERT ( level < levels_count, ("invalid input level", level) );

    if (level < get_control_level(type))
    {
      value = get_value(type, level);
      return true;
    }
    return false;
  }

  void PopValue(uint32_t & type)
  {
    uint8_t const cl = get_control_level(type);
    ASSERT ( cl > 0, (cl) );

    // remove control level
    set_value(type, cl, 0);

    // set control level
    set_value(type, cl-1, 1);
  }
}

namespace 
{
  class suitable_getter
  {
    struct compare_scales
    {
      bool operator() (drule::Key const & l, int r) const { return l.m_scale < r; }
      bool operator() (int l, drule::Key const & r) const { return l < r.m_scale; }
      bool operator() (drule::Key const & l, drule::Key const & r) const { return l.m_scale < r.m_scale; }
    };

    typedef vector<drule::Key> vec_t;
    typedef vec_t::const_iterator iter_t;

    vec_t const & m_rules;
    vec_t & m_keys;

    iter_t m_iters[2];
    int m_scales[2];

    bool m_added;

    void add_rule(int ft, iter_t i)
    {
      static const int visible[3][drule::count_of_rules] = {
        {0, 0, 1, 1, 0, 0, 0},   // fpoint
        {1, 0, 0, 0, 0, 1, 0},   // fline
        {1, 1, 1, 1, 0, 0, 0}    // farea
      };

      if (visible[ft][i->m_type] == 1)
      {
        m_keys.push_back(*i);
        m_added = true;
      }
    }

    void look_forward(int ft)
    {
      if (m_scales[0] < 0) return;
      iter_t i = m_iters[0];
      do
      {
        add_rule(ft, i);
        ++i;
      } while (i != m_rules.end() && i->m_scale == m_scales[0]);
    }

    void look_backward(int ft)
    {
      if (m_scales[1] < 0) return;
      iter_t i = m_iters[1];
      do
      {
        add_rule(ft, i);
        if (i == m_rules.begin())
          break;
        else
          --i;
      } while (i->m_scale == m_scales[1]);
    }

  public:
    suitable_getter(vec_t const & rules, vec_t & keys)
      : m_rules(rules), m_keys(keys)
    {
    }

    void find(int ft, int scale)
    {
      // find greater or equal scale
      m_iters[0] = lower_bound(m_rules.begin(), m_rules.end(), scale, compare_scales());
      if (m_iters[0] != m_rules.end())
        m_scales[0] = m_iters[0]->m_scale;
      else
        m_scales[0] = -1000;

      // if drawing rules exist for 'scale', than process and exit
      if (scale == m_scales[0])
      {
        look_forward(ft);
        return;
      }

      // find less or equal scale
      m_iters[1] = upper_bound(m_rules.begin(), m_rules.end(), scale, compare_scales());
      if (m_iters[1] != m_rules.begin())
      {
        --m_iters[1];
        m_scales[1] = m_iters[1]->m_scale;
      }
      else
        m_scales[1] = -1000;

      // choose the nearest scale to process first
      m_added = false;
      if (abs(m_scales[0] - scale) > abs(m_scales[1] - scale))
      {
        look_backward(ft);
        if (!m_added)
          look_forward(ft);
      }
      else
      {
        look_forward(ft);
        if (!m_added)
          look_backward(ft);
      }
    }
  };
}

void ClassifObject::GetSuitable(int scale, feature_t ft, vector<drule::Key> & keys) const
{
  ASSERT ( ft <= farea, () );

  // 2. Check visibility criterion for scale first.
  if (!m_visibility[scale])
    return;

  // special for AlexZ
  // find rules for 'scale' or if no - for nearest to 'scale' scale
  suitable_getter rulesGetter(m_drawRule, keys);
  rulesGetter.find(ft, scale);
}

bool ClassifObject::IsDrawable(int scale) const
{
  return (m_visibility[scale] && IsDrawableAny());
}

bool ClassifObject::IsDrawableAny() const
{
  return (m_visibility != visible_mask_t() && !m_drawRule.empty());
}

bool ClassifObject::IsDrawableLike(feature_t ft) const
{
  // check the very common criterion first
  if (!IsDrawableAny())
    return false;

  ASSERT ( ft <= farea, () );

  static const int visible[3][drule::count_of_rules] = {
    {0, 0, 1, 1, 0, 0, 0},   // fpoint
    {1, 0, 0, 0, 0, 1, 0},   // fline
    {0, 1, 0, 0, 0, 0, 0}    // farea (!!! key difference with GetSuitable !!!)
  };

  for (size_t i = 0; i < m_drawRule.size(); ++i)
  {
    ASSERT ( m_drawRule[i].m_type < drule::count_of_rules, () );
    if (visible[ft][m_drawRule[i].m_type] == 1)
    {
      /// @todo Check if rule's scale is reachable according to m_visibility (see GetSuitable algorithm).
      return true;
    }
  }

  return false;
}

namespace
{
  bool LoadFileToString(char const * fPath, string & buffer)
  {
    try
    {
      FileReader reader(fPath);
      size_t const sz = static_cast<size_t>(reader.Size());
      if (sz > 0)
      {
        buffer.resize(sz);
        reader.Read(0, &buffer[0], sz);
        return true;
      }
    }
    catch (FileReader::OpenException const &)
    {
      // It's OK. Just return false.
    }
    return false;
  }
}

bool Classificator::ReadClassificator(char const * fPath)
{
  string buffer;
  if (!LoadFileToString(fPath, buffer))
    return false;

  istringstream iss(buffer);

  m_root.Clear();

  ClassifObject::LoadPolicy policy(&m_root);
  tree::LoadTreeAsText(iss, policy);

  m_root.Sort();
  return true;
}

void Classificator::PrintClassificator(char const * fPath)
{
#ifndef OMIM_OS_BADA
  ofstream file(fPath);

  ClassifObject::SavePolicy policy(&m_root);
  tree::SaveTreeAsText(file, policy);

#else
  ASSERT ( false, ("PrintClassificator uses only in indexer_tool") );
#endif
}

bool Classificator::ReadVisibility(char const * fPath)
{
  string buffer;
  if (!LoadFileToString(fPath, buffer))
    return false;

  istringstream iss(buffer);

  ClassifObject::VisLoadPolicy policy(&m_root);
  tree::LoadTreeAsText(iss, policy);

  return true;
}

void Classificator::PrintVisibility(char const * fPath)
{
#ifndef OMIM_OS_BADA
  ofstream file(fPath);

  ClassifObject::VisSavePolicy policy(&m_root);
  tree::SaveTreeAsText(file, policy);

#else
  ASSERT ( false, ("PrintVisibility uses only in indexer_tool") );
#endif
}

void Classificator::SortClassificator()
{
  GetMutableRoot()->Sort();
}

uint32_t Classificator::GetTypeByPath(vector<string> const & path)
{
  ClassifObject const * p = GetRoot();

  size_t i = 0;
  uint32_t type = ftype::GetEmptyValue();

  while (i < path.size())
  {
    ClassifObjectPtr ptr = p->BinaryFind(path[i]);
    ASSERT ( ptr, ("Invalid path in Classificator::GetTypeByPath") );

    ftype::PushValue(type, ptr.GetIndex());

    ++i;
    p = ptr.get();
  }

  return type;
}
