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

unit frmMain;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, ShellAPI, MMSystem, Menus, ImgList, MPlayer;

const
  WM_ICONTRAY = WM_USER+1;

type
  TFormMain = class(TForm)
    PopupMenuTray: TPopupMenu;
    TrayExit: TMenuItem;
    ImageListTray: TImageList;
    TrayAbout: TMenuItem;
    procedure FormCreate(Sender: TObject);
    procedure FormDestroy(Sender: TObject);
    procedure TrayExitClick(Sender: TObject);
    procedure TrayAboutClick(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
    TrayData: TNotifyIconData;
    procedure TrayMessage(var Msg: TMessage); message WM_ICONTRAY;
    procedure StartTrayAnimation;
  end;

var
  FormMain: TFormMain;
  _Ejected: Boolean = False;
  _ThreadCDTerminated: Boolean = False;
  _iTray: Integer;

implementation

uses ThreadCD;

{$R *.dfm}

procedure TFormMain.StartTrayAnimation;
var
  Icon: TIcon;
begin
  Icon:=TIcon.Create;
  _ThreadCDTerminated:=False;
  while _ThreadCDTerminated = False do begin
    {If last icon,start all over,else next icon}
    if _iTray = 3 then _iTray:=0
    else Inc(_iTray);
    {Modify}
    ImageListTray.GetIcon(_iTray,Icon);
    TrayData.hIcon:=Icon.Handle;
    Shell_NotifyIcon(NIM_MODIFY,@TrayData);
    {Sleep 250 ms}
    Sleep(250);
  end;
end;

procedure TFormMain.TrayMessage(var Msg: TMessage);
var
  Pointer: TPoint;
begin
  case Msg.lParam of
    WM_LBUTTONDOWN:
      begin
        TThreadCD.Create(False);
        StartTrayAnimation;
      end;
    WM_RBUTTONDOWN:
      begin
        SetForegroundWindow(Handle);
        GetCursorPos(Pointer);
        PopupMenuTray.Popup(Pointer.X,Pointer.Y);
        PostMessage(Handle, WM_NULL, 0, 0);
      end;
  end;
end;

procedure TFormMain.FormCreate(Sender: TObject);
var
  Icon: TIcon;
begin
  Randomize;
  _iTray := Random(4);
  with TrayData do begin
    cbSize:=SizeOf(TrayData);
    Wnd:=FormMain.Handle;
    uID:=0;
    uFlags:=NIF_MESSAGE+NIF_ICON+NIF_TIP;
    uCallbackMessage:=WM_ICONTRAY;
    StrPCopy(szTip,Application.Title);
      {tray}
      Icon:=TIcon.Create;
      ImageListTray.GetIcon(_iTray,Icon);
      hIcon:=Icon.Handle;
  end;
  Shell_NotifyIcon(NIM_ADD,@TrayData);
end;

procedure TFormMain.FormDestroy(Sender: TObject);
begin
  Shell_NotifyIcon(NIM_DELETE,@TrayData);
end;

procedure TFormMain.TrayExitClick(Sender: TObject);
begin
  Application.Terminate;
end;

procedure TFormMain.TrayAboutClick(Sender: TObject);
begin
  ShowMessage('TrayCD'+#10#13+
              'http://traycd.googlecode.com/');
end;

end.
