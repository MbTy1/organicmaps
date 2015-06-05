#pragma once

#define DATA_FILE_EXTENSION ".mwm"
#define DATA_FILE_EXTENSION_TMP ".mwm.tmp"
#define FONT_FILE_EXTENSION ".ttf"
#define OSM2FEATURE_FILE_EXTENSION ".osm2ft"
#define EXTENSION_TMP ".tmp"

#define DATA_FILE_TAG "dat"
#define GEOMETRY_FILE_TAG "geom"
#define TRIANGLE_FILE_TAG "trg"
#define INDEX_FILE_TAG "idx"
#define SEARCH_INDEX_FILE_TAG "sdx"
#define HEADER_FILE_TAG "header"
#define VERSION_FILE_TAG "version"
#define METADATA_FILE_TAG "meta"
#define METADATA_INDEX_FILE_TAG "metaidx"
#define COMPRESSED_SEARCH_INDEX_FILE_TAG "csdx"

#define ROUTING_MATRIX_FILE_TAG "mercedes"
#define ROUTING_EDGEDATA_FILE_TAG "daewoo"
#define ROUTING_EDGEID_FILE_TAG "infinity"
#define ROUTING_SHORTCUTS_FILE_TAG  "skoda"
#define ROUTING_CROSS_CONTEXT_TAG "chrysler"

#define ROUTING_FTSEG_FILE_TAG  "ftseg"
#define ROUTING_NODEIND_TO_FTSEGIND_FILE_TAG  "node2ftseg"

#define FTSEG_MAPPING_BACKWARD_INDEX_NODES_EXT ".bftsegnodes"
#define FTSEG_MAPPING_BACKWARD_INDEX_BITS_EXT ".bftsegbits"
#define FEATURES_OFFSETS_TABLE_FILE_EXT ".offsets"

//Secret word to unlock experimental features in production builds
#define ROUTING_SECRET_UNLOCKING_WORD "?pedestrian"
#define ROUTING_SECRET_LOCKING_WORD "?vehicle"

// TODO (@ldragunov) change to production server address when we will have one.
#define OSRM_ONLINE_SERVER_URL "http://osrm.online.dev.server"

#define READY_FILE_EXTENSION ".ready"
#define RESUME_FILE_EXTENSION ".resume3"
#define DOWNLOADING_FILE_EXTENSION ".downloading3"
#define BOOKMARKS_FILE_EXTENSION ".kml"
#define ROUTING_FILE_EXTENSION ".routing"

#define GEOM_INDEX_TMP_EXT ".geomidx.tmp"
#define CELL2FEATURE_SORTED_EXT ".c2f.sorted"
#define CELL2FEATURE_TMP_EXT ".c2f.tmp"

#define COUNTRIES_FILE  "countries.txt"

#define WORLD_FILE_NAME "World"
#define WORLD_COASTS_FILE_NAME "WorldCoasts"

#define SETTINGS_FILE_NAME "settings.ini"

#define SEARCH_CATEGORIES_FILE_NAME "categories.txt"

#define PACKED_POLYGONS_FILE "packed_polygons.bin"
#define PACKED_POLYGONS_INFO_TAG "info"

#define EXTERNAL_RESOURCES_FILE "external_resources.txt"

/// How many langs we're supporting on indexing stage
#define MAX_SUPPORTED_LANGUAGES 64


