pieces = [
    "pawn_white", "rook_white", "knight_white", "bishop_white", "king_white", "queen_white",
    "pawn_black", "rook_black", "knight_black", "bishop_black", "king_black", "queen_black"
]

for piece in pieces:
    print(f'Reading {piece}')
    xs, ys, zs = [], [], []
    with open(f'{piece}/model.obj', 'r') as source:
        for line in source.readlines():
            try:
                t, *items = line.split()
                if t == 'v':
                    x, y, z = map(float, items)
                    xs.append(x)
                    ys.append(y)
                    zs.append(z)
            except ValueError:
                pass
    
    xm = min(xs)
    xM = max(xs)
    ym = min(ys)
    yM = max(ys)
    zm = min(zs)
    zM = max(zs)

    print(f'Writing {piece}')
    with open(f'{piece}/model_small.obj', 'r') as source:
        with open(f'{piece}/modelf.obj', 'w') as destination:
            for line in source.readlines():
                try:
                    t, *items = line.split()
                    if t == 'v':
                        x, y, z = map(float, items)
                        x -= (xm+xM)/2
                        y -= (ym+yM)/2
                        z -= zm
                        destination.write(f'v {x:.2f} {y:.2f} {z:.2f}\n')
                    else:
                        destination.write(line) 
                except ValueError:
                    destination.write(line) 

print('Done')
