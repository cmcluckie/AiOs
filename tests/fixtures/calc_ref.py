import sys
def w32(x): x&=0xffffffff; return x-(1<<32) if x>=1<<31 else x
def run(inp):
    out=["RPN calculator.  Numbers and + - * / %, dup swap drop clear, q to quit.\n> "]
    st=[]; buf=""; last_cr=False
    def line(s):
        for t in s.split():
            # number?
            body=t[1:] if (t.startswith('-') and len(t)>1) else t
            if body.isdigit() and body.isascii():
                v=int(t)
                if v>2147483647 or v< -2147483648: out.append("error: too big\n"); return
                if len(st)==64: out.append("error: stack full\n"); return
                st.append(v); continue
            if t in('q','quit'): out.append("bye\n"); raise SystemExit
            if t=='clear': st.clear(); continue
            if t=='drop':
                if len(st)<1: out.append("error: stack empty\n"); return
                st.pop(); continue
            if t=='dup':
                if len(st)<1: out.append("error: stack empty\n"); return
                if len(st)==64: out.append("error: stack full\n"); return
                st.append(st[-1]); continue
            if t=='swap':
                if len(st)<2: out.append("error: stack empty\n"); return
                st[-1],st[-2]=st[-2],st[-1]; continue
            if t in '+-*/%' and len(t)==1:
                if len(st)<2: out.append("error: stack empty\n"); return
                a,b=st[-2],st[-1]
                if t in '/%' and b==0: out.append("error: divide by zero\n"); return
                if t in '/%' and a==-2**31 and b==-1: out.append("error: overflow\n"); return
                if t=='+': v=w32(a+b)
                elif t=='-': v=w32(a-b)
                elif t=='*': v=w32(a*b)
                else:
                    q=abs(a)//abs(b); q=q if (a<0)==(b<0) else -q
                    v=q if t=='/' else a-b*q
                st.pop(); st[-1]=v; continue
            out.append(f"error: unknown '{t}'\n"); return
        out.append(f"= {st[-1]}\n" if st else "(empty)\n")
    try:
        for c in inp:
            if c=='\n' and last_cr: last_cr=False; continue
            last_cr=(c=='\r')
            if c in '\r\n':
                out.append("\n"); line(buf); buf=""; out.append("> ")
            elif c in '\x03\x04': out.append("\n"); out.append("bye\n"); raise SystemExit
            elif c in '\x08\x7f':
                if buf: buf=buf[:-1]; out.append("\b \b")
            elif ' '<=c<='~' and len(buf)<80: buf+=c; out.append(c)
    except SystemExit: pass
    return ''.join(out)
sys.stdout.write(run(open(sys.argv[1],newline='').read()))
