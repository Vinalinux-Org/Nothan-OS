#!/bin/sh
# Chay app. Dung Python trong .venv cua thu muc nay, khong dung Python he thong.
cd "$(dirname "$0")" && exec .venv/bin/python main.py "$@"
