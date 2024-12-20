/*
_____________________________________________________________________________

                       File Association
_____________________________________________________________________________

 Based on code taken from http://nsis.sourceforge.net/File_Association

 Usage in script:
 1. !include "FileAssociation.nsh"
 2. [Section|Function]
      ${FileAssociationFunction} "Param1" "Param2" "..." $var
    [SectionEnd|FunctionEnd]

 FileAssociationFunction=[RegisterExtension|UnRegisterExtension]

_____________________________________________________________________________

 ${RegisterExtension} "[executable]" "[extension]" "[description]"

"[executable]"     ; executable which opens the file format
                   ;
"[extension]"      ; extension, which represents the file format to open
                   ;
"[description]"    ; description for the extension. This will be display in Windows Explorer.
                   ;


 ${UnRegisterExtension} "[extension]" "[description]"

"[extension]"      ; extension, which represents the file format to open
                   ;
"[description]"    ; description for the extension. This will be display in Windows Explorer.
                   ;

_____________________________________________________________________________

                         Macros
_____________________________________________________________________________

 Change log window verbosity (default: 3=no script)

 Example:
 !include "FileAssociation.nsh"
 !insertmacro RegisterExtension
 ${FileAssociation_VERBOSE} 4   # all verbosity
 !insertmacro UnRegisterExtension
 ${FileAssociation_VERBOSE} 3   # no script
*/


!ifndef FileAssociation_INCLUDED
!define FileAssociation_INCLUDED

!include Util.nsh

!verbose push
!verbose 3
!ifndef _FileAssociation_VERBOSE
  !define _FileAssociation_VERBOSE 3
!endif
!verbose ${_FileAssociation_VERBOSE}
!define FileAssociation_VERBOSE `!insertmacro FileAssociation_VERBOSE`
!verbose pop

!macro FileAssociation_VERBOSE _VERBOSE
  !verbose push
  !verbose 3
  !undef _FileAssociation_VERBOSE
  !define _FileAssociation_VERBOSE ${_VERBOSE}
  !verbose pop
!macroend

!macro RegisterExtensionCall _EXECUTABLE _EXTENSION _APPNAME _DESCRIPTION _ICON
  !verbose push
  !verbose ${_FileAssociation_VERBOSE}
  Push `${_DESCRIPTION}`
  Push `${_APPNAME}`
  Push `${_EXTENSION}`
  Push `${_EXECUTABLE}`
  Push `${_ICON}`
  ${CallArtificialFunction} RegisterExtension_
  !verbose pop
!macroend

!macro UnRegisterExtensionCall _EXTENSION _APPNAME
  !verbose push
  !verbose ${_FileAssociation_VERBOSE}
  Push `${_EXTENSION}`
  Push `${_APPNAME}`
  ${CallArtificialFunction} UnRegisterExtension_
  !verbose pop
!macroend



!define RegisterExtension `!insertmacro RegisterExtensionCall`
!define un.RegisterExtension `!insertmacro RegisterExtensionCall`

!macro RegisterExtension
!macroend

!macro un.RegisterExtension
!macroend

!macro RegisterExtension_
  !verbose push
  !verbose ${_FileAssociation_VERBOSE}

  Exch $R4 ; Save value of $R3 and replace with ${_ICON}
  Exch
  Exch $R3 ; Save value of $R3 and replace with ${_EXECUTABLE}
  Exch
  Exch $R2 ; Save value of $R2 and replace with ${_EXTENSION}
  Exch
  Exch 2
  Exch $R1 ; Save value of $R1 and replace with {_APPNAME}
  Exch 2
  Exch 3
  Exch $R0 ; Save value of $R0 and replace with ${_DESCRIPTION}
  Exch 3
  Push $0 ; Save value of $0
  Push $1 ; Save value of $1

;; Make a backup of the previous key
; Find the idea nice, but never saw an application doing that.. Let's skip it!
;
;  ReadRegStr $1 HKCR "$R2" ""  ; read current file association
;  StrCmp "$1" "" NoBackup  ; If it is not set..
;  StrCmp "$1" "$R1" NoBackup  ; or the same value, as ours now,
;  WriteRegStr HKCR "$R2" "backup_val" "$1"  ; then backup the current value.
;NoBackup:
  ; Setting default association
  WriteRegStr HKCR "$R2" "" "$R1"
  ; Also an instruction to set the default association for Explorer
  ; The change takes effect after restarting explorer.exe
  DeleteRegKey /ifempty HKCR "Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\$R2"

  ReadRegStr $0 HKCR "$R1" ""
  StrCmp $0 "" 0 Skip
    WriteRegStr HKCR "$R1" "" "$R0"
    WriteRegStr HKCR "$R1\shell" "" "open"
    WriteRegStr HKCR "$R1\DefaultIcon" "" "$R3,$R4"
Skip:
  WriteRegStr HKCR "$R1\shell\open\command" "" '"$R3" "%1"'
  WriteRegStr HKCR "$R2\OpenWithProgids" "$R1" "" ; Setting a OpenWith entry

; Uncomment if you want a "Edit with XY" entry
;  WriteRegStr HKCR "$R1\shell\edit" "" "Edit $R0"
;  WriteRegStr HKCR "$R1\shell\edit\command" "" '"$R3" "%1"'

; Uncomment if you want a "Print with XY" entry.. Woohoom, even better..
;  WriteRegStr HKCR "$R1\shell\print" "" "Print $R0"
;  WriteRegStr HKCR "$R1\shell\print\command" "" '"$R3" "%1"'

; Uncomment if you want a "Slice with Creality Slicer".. Oh my god!
;  WriteRegStr HKCR "$R1\shell\slice" "" "Slice with Creality Slicer"
;  WriteRegStr HKCR "$R1\shell\slice\command" "" '"$R3" "%1"'

  Pop $1 ; Recover value of $1
  Pop $0 ; Recover value of $0
  Pop $R3 ; Recover value of $R3
  Pop $R2 ; Recover value of $R2
  Pop $R1 ; Recover value of $R1
  Pop $R0 ; Recover value of $R0
!macroend

!define UnRegisterExtension `!insertmacro UnRegisterExtensionCall`
!define un.UnRegisterExtension `!insertmacro UnRegisterExtensionCall`

!macro UnRegisterExtension
!macroend

!macro un.UnRegisterExtension
!macroend

!macro UnRegisterExtension_
  !verbose push
  !verbose ${_FileAssociation_VERBOSE}

  Exch $R1 ; Save value of $R1 and replace with ${_APPNAME}
  Exch
  Exch $R0 ; Save value of $R0 and replace with ${_EXTENSION}
  Exch
  Push $0 ; Save value of $0
  Push $1 ; Save value of $1

  DeleteRegKey HKCR "$R0\OpenWithProgids\$R1" ; Delete "Open with.." entry

;; Backing up previous value
; Just like I mentioned before. Unneeded stuff..
;  ReadRegStr $1 HKCR "$R0" ""
;  StrCmp $1 "$R1" 0 NoOwn ; only do this if we own it
;  ReadRegStr $1 HKCR $R0 "backup_val"
;  StrCmp $1 "" 0 Restore ; if backup="" then delete the whole key
  DeleteRegKey /ifempty HKCR "$R0\OpenWithProgids"
  WriteRegStr HKCR "$R0" "" ""  ; BETTER: Setting it to nothing, if it was ours. The user can choose what she/he wants to use afterwards
  ; Emptying the key - In my opinion safer then removing it.
  ; However, see above to see why this is turned off.
  ;WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\$R2\UserChoice" "ProgId" ""
  DeleteRegKey /ifempty HKCR "$R0"
;  Goto NoOwn

;Restore:
;  WriteRegStr HKCR "$R0" "" $1 ; Set back'ed up value
;  DeleteRegValue HKCR "$R0" "backup_val" ; Remove the backup
;  DeleteRegKey HKCR "$R1" ; Delete key with association name settings
;  Goto NoOwn

;NoOwn:

  Pop $1 ; Recover value of $1
  Pop $0 ; Recover value of $0
  Pop $R1 ; Recover value of $R1
  Pop $R0 ; Recover value of $R0

  !verbose pop
!macroend

!endif # !FileAssociation_INCLUDED