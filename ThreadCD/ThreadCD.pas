{
  TrayCD
  Copyright (C) 2008  Stefan Sundin

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
}

unit ThreadCD;

interface

uses
  Classes, frmMain, MMSystem;

type
  TThreadCD = class(TThread)
  protected
    procedure Execute; override;
  end;

implementation

procedure TThreadCD.Execute;
begin
  FreeOnTerminate:=True;
  if _Ejected = False then begin
    mciSendString('Set cdaudio door open wait',nil,0,handle);
    _Ejected:=True;
  end
  else begin
    mciSendString('Set cdaudio door closed wait',nil,0,handle);
    _Ejected:=False;
  end;
  _ThreadCDTerminated:=True;
end;

end.
