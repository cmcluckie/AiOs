# a stand-in for jig/judge in tests: a file with GOOD in it passes
[ -f "$2/part.c" ] || { echo "no part.c"; exit 1; }
case "$(cat "$2/part.c")" in *GOOD*) echo "pass 5 lines"; exit 0 ;; esac
echo "fail 7 lines: serial.waits-ready@1"; exit 1
