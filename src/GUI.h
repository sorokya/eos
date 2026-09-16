#ifndef GUIH
#define GUIH

#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>

class TGUI : public TForm
{
__published:
private:
public:
    __fastcall TGUI(TComponent* Owner);
};

extern PACKAGE TGUI *GUI;

#endif
