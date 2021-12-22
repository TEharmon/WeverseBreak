import sys
sBox  = [0x9, 0x4, 0xa, 0xb, 0xd, 0x1, 0x8, 0x5,0x6, 0x2, 0x0, 0x3, 0xc, 0xe, 0xf, 0x7]
sBoxI = [0xa, 0x5, 0x9, 0xb, 0x1, 0x7, 0x8, 0xf,0x6, 0x0, 0x2, 0x3, 0xc, 0x4, 0xd, 0xe]
w = [None] * 6
def mult(p1,p2):
    p=0
    while p2:
        if p2 & 0b1:
            p^=p1
        p1<<=1
        if p1 & 0b10000:
            p1^=0b11
        p2>>=1
    return p & 0b1111

def convertVector(n):
    return [n>>12, (n>>4) & 0xf, (n>>8) & 0xf, n & 0xf]

def covertInteger(x):
    return (x[0]<<12)+(x[1]<<4)+(x[2]<<8)+x[3]

def addKey(s1, s2):
    return [i ^ j for i, j in zip(s1, s2)]

def substitution(sbox, s):
    return [sbox[e] for e in s]

def shift(s):
    return [s[0], s[1], s[3], s[2]]

def keyExpansion(key):
    def sub2Nib(B):
        return sBox[b >> 4] + (sBox[b & 0x0f] << 4)
    Rcon1, Rcon2 = 0b10000000, 0b00110000
    w[0] = (key & 0xff00) >> 8
    w[1] = key & 0x00ff
    w[2] = w[0] ^ Rcon1 ^ sub2Nib(w[1])
    w[3] = w[2] ^ w[1]
    w[4] = w[2] ^ Rcon2 ^ sub2Nib(w[3])
    w[5] = w[4] ^ w[3]

def encrypt(ptext):
    def mixColumns(s):
        return [s[0] ^ mult(4, s[2]), s[1] ^ mult(4, s[3]),
                s[2] ^ mult(4, s[0]), s[3] ^ mult(4, s[1])]  
      
    state = convertVector(((w[0] << 8) + w[1]) ^ ptext)
    state = mixColumns(shift(substitution(sBox, state)))
    state = addKey(convertVector((w[2] << 8) + w[3]), state)
    state = shift(substitution(sBox, state))
    return convertInteger(addKey(convertVector((w[4] << 8) + w[5]), state))

def decrypt(ctext):
    #function used to mix the columns of the two matrixs by [9, 2, 2, 9]
    def inverseMixColumns(s):
        return [mult(9, s[0]) ^ mult(2, s[2]), mult(9, s[1]) ^ mult(2, s[3]),
                mult(9, s[2]) ^ mult(2, s[0]), mult(9, s[3]) ^ mult(2, s[1])]
      
    state = convertVector(((w[4] << 8) + w[5]) ^ ctext)
    state = substitution(sBoxI, shift(state))
    state = inverseMixColumns(addKey(convertVector((w[2] << 8) + w[3]), state))
    state = substitution(sBoxI, shift(state))
    return convertInteger(addKey(convertVector((w[0] << 8) + w[1]), state))
  
#main function
if __name__ == '__main__':
    print('This program encrypts your 16-bit binary plaintext and key using')
    print('s-AES (simplified advanced encryption standard)')
    print('\n')
 
    #used to convert to binary
    getBin = lambda x: x >= 0 and str(bin(x))[2:] or "-" + str(bin(x))[3:]

    #choice variable to select option from user
    choice = input('Press 1 to encrypt your 16-bit plaintext, 2 for decryption'
                   + ', and 3 to exit.\n')
     
    if (choice == 1):
        print('\nPlease note that you must include 0b (to represent binary)'
              + ' before your inputs')
        plaintext=input('Please enter your plaintext (ex: 0b1101011100101000): ')
        key=input('Please enter your key (ex: 0b0100101011110101): ')
 
        #uses key input and expands, creating other keys
        keyExpansion(key)
        print('Your encrypted plaintext is: ')
        print(getBin(encrypt(plaintext)))
 
    elif (choice == 2):
        print('\nPlease note that you must include 0b (to represent binary)'
              + ' before your inputs')
        ciphertext=input('Please enter your ciphertext (ex: 0b0010010011101100): ')
        key=input('Please enter your key (ex: 0b0100101011110101): ')
 
        #uses key input and expands, creating other keys
        keyExpansion(key)
        print('Your decrypted ciphertext is: ')
        print(getBin(decrypt(ciphertext)))   
    elif (choice == 0):
        print('Exiting program')
        sys.exit(1)
    else:
        print('Invaild selection')
