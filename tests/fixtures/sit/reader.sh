# a stand-in for jig/reader in tests: answers pin, echoes anything else
case "$1" in pin) echo stubpin ;; read) echo "$2:$3: stub line from the reader" ;; *) echo "stub $*" ;; esac
