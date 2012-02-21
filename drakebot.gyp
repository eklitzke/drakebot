{
  'target_defaults': {
    'type': 'executable',
    'cflags': ['-Wall'],
    'conditions': [
       ['OS=="linux"', {
         'ldflags': [
           '-pthread',
          ],
          'libraries': [
            '-lboost_system',
            '-lcrypto',
            '-lgflags',
            '-lglog',
            '-lssl',
          ]
    }]],
    'sources': [
      'bot.cc',
      'main.cc'
    ]
  },
  'targets': [
    {
      # drakebot with debugging symbols, logging
      'target_name': 'drakebot',
      'cflags': ['-g'],
      'defines': [ 'DEBUG', ],
    },
    {
      # optimized drakebot
      'target_name': 'drakebot_opt',
      'cflags': ['-fomit-frame-pointer'],
      'defines': [ 'NDEBUG', ],
      'postbuilds': [
        {
          'postbuild_name': 'strip optimized binary',
          'action': [
            'strip',
            '-s',
            '$(BUILT_PRODUCTS_DIR)/$(EXECUTABLE_PATH)'],
        },
      ],
    },
  ],
}
