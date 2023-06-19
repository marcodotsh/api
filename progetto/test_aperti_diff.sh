#!/usr/bin/env bash

mkfifo main_output

./main < archivio_test_aperti/open_$1.txt > main_output &
PID=$!

diff --color=auto main_output archivio_test_aperti/open_$1.output.txt

rm main_output

echo "Process PID was $PID"

