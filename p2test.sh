#!/bin/bash

# USYD CODE CITATION ACKNOWLEDGEMENT
# I declare that the majority of the python3 script and inspiration comes from the following two links
# https://stackoverflow.com/questions/12605498/how-to-use-subprocess-popen-python
# https://stackoverflow.com/questions/89228/how-do-i-execute-a-program-or-call-a-system-command

# there are lots of race conditions so generally the output will not match
# unless run many many times and one time it might match

cd p2tests/test1
python3 test1.py
cd ../test2
python3 test2.py