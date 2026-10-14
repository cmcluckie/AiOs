# Shared by the editor's .check files: sourced, not run.
# frames CAPTURE: every screen the editor drew, one per record, separated by a
# form feed, escape codes taken out and rows joined by newlines.  The editor
# starts each frame with ESC [ ? 25 l (cursor off while drawing).
frames() {
	awk 'BEGIN { RS = "\033\\[\\?25l" } NR > 1 { printf "%s\f", $0 }' "$1" |
		sed 's/\x1b\[[0-9;?]*[A-Za-z]//g' | tr -d '\r'
}
# last CAPTURE: the last frame, its text rows then its bottom line
last() { frames "$1" | awk 'BEGIN { RS = "\f" } { f = $0 } END { print f }'; }
# text CAPTURE: the last frame's text, trailing empty rows dropped
text() { last "$1" | sed '$d' | sed -e :a -e '/^\n*$/{$d;N;ba' -e '}'; }
# bottom CAPTURE: the last frame's bottom line
bottom() { last "$1" | tail -1; }
