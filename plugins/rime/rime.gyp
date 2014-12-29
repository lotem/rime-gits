{
  'targets': [
    {
      'target_name': 'rime_plugin',
      'type': 'shared_library',
      'include_dirs': [
        '<(SHARED_INTERMEDIATE_DIR)/protoc_out',
      ],
      'sources': [
        'keysym.h',
        'rime_key_event.cc',
        'rime_key_event.h',
        'rime_engine_component.cc',
        'rime_engine_component.h',
        'rime_plugin.cc',
      ],
      'msvs_settings': {
        'VCLinkerTool': {
          'ModuleDefinitionFile': 'rime.def',
        },
      },
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/common/common.gyp:common',
        '<(DEPTH)/components/common/common.gyp:component_common',
        '<(DEPTH)/components/plugin_wrapper/plugin_wrapper.gyp:plugin_wrapper',
        '<(DEPTH)/ipc/ipc.gyp:ipc',
        '<(DEPTH)/ipc/protos/protos.gyp:protos-cpp',
		'<(DEPTH)/skin/skin.gyp:skin',
      ],
    },
  ]
}
