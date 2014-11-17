{
    'targets': [{
        'target_name': 'node_quadtree',
        'sources': [
            'src/quadtree.cc',
        ],
        'conditions': [[
            'OS=="mac"', {
            }
        ], [
            'OS=="linux" and target_arch=="ia32"', {
            }
        ], [
            'OS=="linux" and target_arch=="x64"', {
            }
        ]],
        'defines': [
        ]
    }]
}
