;TrayCD - gl-ES localization by Alexander De Sousa (xandy.lua@gmail.com)
;This file is ANSI encoded.
;
;Copyright (C) 2009  Stefan Sundin (recover89@gmail.com)
;
;This program is free software: you can redistribute it and/or modify
;it under the terms of the GNU General Public License as published by
;the Free Software Foundation, either version 3 of the License, or
;(at your option) any later version.

!if ${L10N_VERSION} == 2

!insertmacro MUI_LANGUAGE "Galician"
!define LANG ${LANG_GALICIAN}

LangString L10N_UPGRADE_TITLE     ${LANG} "Xa est� instalado."
LangString L10N_UPGRADE_SUBTITLE  ${LANG} "Escolle como queres instalar ${APP_NAME}."
LangString L10N_UPGRADE_HEADER    ${LANG} "${APP_NAME} xa est� instalado no teu sistema. Selecciona a operaci�n que queres realizar e preme Seguinte para continuar."
LangString L10N_UPGRADE_UPGRADE   ${LANG} "&Actualizar ${APP_NAME} � versi�n ${APP_VERSION}."
LangString L10N_UPGRADE_INI       ${LANG} "A configuraci�n actual copiarase a ${APP_NAME}-old.ini."
LangString L10N_UPGRADE_INSTALL   ${LANG} "&Instalar de novo."
LangString L10N_UPDATE_SECTION    ${LANG} "Comprobar actualizaci�ns antes de instalar"
LangString L10N_UPDATE_DIALOG     ${LANG} "Unha nova versi�n est� dispo�ible.$\nQueres abortar a instalaci�n e ir � p�xina web?"
LangString L10N_RUNNING           ${LANG} "${APP_NAME} est� en funcionamento. Qu�relo pechar?"
LangString L10N_RUNNING_UNINSTALL ${LANG} "Se seleccionas Non, ${APP_NAME} eliminarase completamente cando reinicies."
LangString L10N_SHORTCUT          ${LANG} "Atallo no men� de inicio"
LangString L10N_AUTOSTART         ${LANG} "Arrancar � iniciar o sistema"

!undef LANG

!else
!warning "Localization out of date!"
!endif
