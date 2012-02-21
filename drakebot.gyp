{
  'targets': [
    {
      'target_name': 'drakebot',
      'type': 'executable',
      'cflags': ['-g'],
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
      'defines': [
        'DEFINE_DEBUG',
      ],
      'sources': [
        'bot.cc',
        'main.cc'
      ]
    },
  ]
}
