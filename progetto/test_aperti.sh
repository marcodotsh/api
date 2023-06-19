#!/usr/bin/env bash

mkfifo main_output

./main < archivio_test_aperti/open_$1.txt > main_output &
PID=$!

diff --side-by-side main_output archivio_test_aperti/open_$1.output.txt | nl

rm main_output

echo "Process PID was $PID"

