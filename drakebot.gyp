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
		    '-lboost_program_options',
		    '-lcrypto',
            '-lglog',
            '-lssl',
          ]
        }]],
      'defines': [
        'DEFINE_DEBUG',
      ],
      'sources': [
        'main.cc',
        #'line_reader.cc',
        'bot.cc',
      ]
    }
  ]
}
