import sys
import hashlib
import os
import getopt

number_bytes = 0
outfile = ''

try:
    opts, args = getopt.getopt(sys.argv[1:], 'n:o:')
except getopt.GetoptError:
    print(
        'Error: Both -n <number of bytes> and -o <output file>  are required')
    sys.exit(1)

for opt, arg in opts:
    if opt == '-n':
        number_bytes = int(arg.strip())
    elif opt == '-o':
        outfile = arg

with open(outfile, 'wb') as file:
    file.write(os.urandom(number_bytes))

sha = hashlib.sha512()
blake = hashlib.blake2b()
buffer_size = 1048576

with open(outfile, 'rb') as file:
    while True:
        data = file.read(buffer_size)
        if not data:
            break
        sha.update(data)
        blake.update(data)

print('SHA512:{0}'.format(sha.hexdigest()))
print('BLAKE2B:{0}'.format(blake.hexdigest()))
