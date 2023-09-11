import argparse
import os
import sys

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-i", help="Image file to install bootloader", dest="imagePath", required=True)
    parser.add_argument("-b", help="Bootloader binary to install", dest="binaryPath", required=True)
    parser.add_argument("-o", help="Output file, write back to image if not provided", dest="outputPath")
    
    args = parser.parse_args()

    bootSize = os.path.getsize(args.binaryPath)
    imageSize = os.path.getsize(args.imagePath)

    print("Boot binary size: {0}\nImage size: {1}\n".format(bootSize, imageSize))

    if bootSize > (1 << 20):
        sys.exit("Bootloader binary file is too large!")
    
    with open(args.imagePath, "rb") as image:
        image.seek(0, 0)
        imageMBR = image.read(512)
        if imageMBR[510:512].hex() != "55aa":
            sys.exit("Invalid Image!")

        bootFound = False
        firstPartition = -1
        for i in range(0, 4):
            entryPos = 446 + i * 16
            partitionEntry = imageMBR[entryPos:entryPos + 16]
            if int(partitionEntry[4]) == 0:
                continue

            if firstPartition == -1:
                firstPartition = i

            bootFound = bootFound or int(partitionEntry[0]) == int("0x80", 16)
            print("Found partition {0}, begin: 0x{1}, length: 0x{2}, type: 0x{3}, boot: 0x{4}\n".format(i, partitionEntry[8:12].hex(), partitionEntry[12:16].hex(), partitionEntry[4:5].hex(), partitionEntry[0:1].hex()))

        if firstPartition == -1:
            print("WARNING: no partition found")
        
    with open(args.binaryPath, "rb") as binary:
        binary.seek(0, 0)
        MBR = binary.read(512)

        if MBR[510:512].hex() != "55aa":
            sys.exit("Invalid bootloader MBR!")

        bootloader = binary.read(bootSize - 512)

    with open(args.imagePath if args.outputPath == None or args.outputPath == args.imagePath else args.outputPath, "rb+") as output:
        output.seek(0, 0)
        output.write(MBR[:440])
        print("Installing MBR")

        if args.outputPath != None and args.outputPath != args.imagePath:
            print("Copying partition table")
            with open(args.imagePath, "rb") as image:
                image.seek(440, 0)
                partitionTable = image.read(72)
                output.seek(440, 0)
                output.write(partitionTable)

        if not bootFound and firstPartition != -1:
            output.seek(446 + firstPartition * 16 + 0, 0)
            output.write(b"\x80")
            print("No bootable partition found, setting first partition to bootable")

        output.seek(512, 0)
        output.write(bootloader)
        print("Installing bootloader")

        if args.outputPath != None and args.outputPath != args.imagePath:
            print("Copying partitions")
            with open(args.imagePath, "rb") as image:
                image.seek(1 << 20, 0)
                partitions = image.read()
                output.seek(1 << 20, 0)
                output.write(partitions)

    print("Bootloader install complete!")

if __name__ == '__main__':
    main()