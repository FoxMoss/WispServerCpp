
{
  "targets": [
    {
      "target_name": "wispservercpp",
      "sources": [ "nodeBinding.cpp", "../interface.cpp", "../socketManager.cpp" ],
      "defines": [ "V8_DEPRECATION_WARNINGS=1", "NODE_ENV=true"],
      'dependencies': [
        "<!(node -p \"require('node-addon-api').targets\"):node_addon_api",
      ],
    }
  ]
}
