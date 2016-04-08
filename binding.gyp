{
    'targets': [
        {
            'target_name': 'node_quadtree',
            'sources': [
                'src/quadtree.cc',
            ],
            "include_dirs": [
                "<!(node -e \"require('nan')\")"
            ],
            'conditions': [
                [
                    'OS=="mac"', {}
                ],
                [
                    'OS=="linux" and target_arch=="ia32"', {}
                ],
                [
                    'OS=="linux" and target_arch=="x64"', {}
                ]
            ],
            'defines': [
            ]
        }
    ]
}
