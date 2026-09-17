struct MO { short x; short y; short v; MO(short, short, short); };
MO::MO(short a, short b, short c) { x=a; y=b; v=c; }
void f1(short x, short y, short v) { MO o(x, y, v); }
void f2(int x, int y, int v) { MO o(x, y, v); }
