nf = open('D:\\Link\\3D Objects\\向晚\\Xiangwan_texc_01.obj', 'w')
with open('D:\\Link\\3D Objects\\向晚\\XiangWan.obj') as f:
    for line in f.readlines():
        if line.startswith('vt'):
            l = line.split(' ')
            l[1] = str(round(float(l[1]) % 1.0, 6))
            l[2] = str(round(1 - float(l[2]) % 1.0, 6))
            line = ' '.join(l)
            nf.write(line + '\n')
        else:
            nf.write(line)
nf.close()
