import argparse
import os
import sys
import re
import fileinput

def fixHeaderFile(base : str, fullPath : str, verbose: bool):
    if not fullPath.startswith(base):
        print('Path not begin with base!')
        sys.exit(-1)
        
    newGuard = fullPath[len(base) + 1:]
    newGuard = newGuard.upper()
    newGuard = '__' + re.sub('[^0-9a-zA-Z]+', '_', newGuard)
    
    with open(fullPath, 'r') as f:
        content = f.read()
        
    match = re.search(r'#if\s+!defined\s*\((\w+)\)\s*.*\n#define\s+\w+', content, re.DOTALL)
    if match:
        oldGuard = match.group(1)

        if oldGuard == newGuard:
            if verbose:
                print('{}: Not changed'.format(fullPath))
            return
        
        replaced = match.group(0).replace(oldGuard, newGuard)
        content = content.replace(match.group(0), replaced, 1)
        
        content = re.sub(r'#endif\s+//\s*{}'.format(oldGuard), r'#endif // {}'.format(newGuard), content, 1)
        
        print('{}: {} --> {}'.format(fullPath, oldGuard, newGuard))
    else:
        print('{}: Not matched'.format(fullPath))
        
    with open(fullPath, 'w') as file:
            file.write(content)
def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-b", help = "Base path of the include directory containing headers to fix", dest = "base", required = True)
    parser.add_argument("-v", help = "verbose output", dest = "verbose", default = False, action = "store_true")
    
    args = parser.parse_args()
    
    base = args.base
    if base.endswith('/'):
        base = base[:-1]
    
    verbose = args.verbose
    
    for root, _, files in os.walk(base, topdown=False):
        for f in files:
            if not f.endswith('.h'):
                continue
            
            filePath = os.path.join(root, f)
            fixHeaderFile(base, filePath, verbose)
            
            
    
if __name__ == '__main__':
    main()