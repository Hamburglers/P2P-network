# USYD CODE CITATION ACKNOWLEDGEMENT
# I declare that the majority of the python3 script and inspiration comes from the following two links
# https://stackoverflow.com/questions/12605498/how-to-use-subprocess-popen-python
# https://stackoverflow.com/questions/89228/how-do-i-execute-a-program-or-call-a-system-command

import subprocess
import filecmp
import os
import time
import tempfile

def run_btide(config_file, input_file, output_file):
    with open(input_file, 'r') as infile, open(output_file, 'w') as outfile:
        process = subprocess.Popen(['../../btide', config_file], stdin=infile, stdout=outfile, stderr=subprocess.PIPE)
        return process

def compare_files(file1, file2):
    if filecmp.cmp(file1, file2, shallow=False):
        print(f"The outputs match for {file1} and {file2}!")
    else:
        print(f"The outputs do not match for {file1} and {file2}.")

def main():
    with tempfile.NamedTemporaryFile(delete=False) as temp_out1, \
         tempfile.NamedTemporaryFile(delete=False) as temp_out2, \
         tempfile.NamedTemporaryFile(delete=False) as temp_out3:
        temp_out1_name = temp_out1.name
        temp_out2_name = temp_out2.name
        temp_out3_name = temp_out3.name

    try:
        peer1_process = run_btide('peer1.cfg', 'test2peer1.in', temp_out1_name)
        peer2_process = run_btide('peer2.cfg', 'test2peer2.in', temp_out2_name)
        peer3_process = run_btide('peer3.cfg', 'test2peer3.in', temp_out3_name)
        
        peer1_process.wait()
        peer2_process.wait()
        peer3_process.wait()
        
        compare_files(temp_out1_name, 'test2peer1.out')
        compare_files(temp_out2_name, 'test2peer2.out')
        compare_files(temp_out3_name, 'test2peer3.out')
    finally:
        os.remove(temp_out1_name)
        os.remove(temp_out2_name)
        os.remove(temp_out3_name)

if __name__ == '__main__':
    main()
