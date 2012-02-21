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
      'defines': [ 'DEFINE_DEBUG', ],
    },
    {
      # optimized drakebot
      'target_name': 'drakebot_opt',
      'defines': [ 'DEFINE_NDEBUG', ],
    },
  ],
}
