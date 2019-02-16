{
  "targets": [
    {
      "target_name": "altconfig",
      "sources": [ "src/main.cpp" ],
      "include_dirs": [ "deps/alt-config/" ],
      "cflags_cc": [
        "-std=c++17"
      ],
      'msvs_settings': {
        'VCCLCompilerTool': {
          'AdditionalOptions': [ '-std:c++17', ],
        },
      }
    }
  ]
}