;TrayCD - es-ES localization by Fabrizio Ferri (algernon@gmail.com)
;
;This program is free software: you can redistribute it and/or modify
;it under the terms of the GNU General Public License as published by
;the Free Software Foundation, either version 3 of the License, or
;(at your option) any later version.

!insertmacro MUI_LANGUAGE "Spanish"
!define LANG ${LANG_SPANISH}

LangString L10N_UPGRADE_TITLE     ${LANG} "Ya est� instalado"
LangString L10N_UPGRADE_SUBTITLE  ${LANG} "Elige el modo de instalaci�n de ${APP_NAME}."
LangString L10N_UPGRADE_HEADER    ${LANG} "${APP_NAME} ya se encuentra instalado. Selecciona la operaci�n que quieres llevar a cabo y pulsa Siguiente para continuar."
LangString L10N_UPGRADE_UPGRADE   ${LANG} "&Actualiza ${APP_NAME} a la versi�n ${APP_VERSION}."
LangString L10N_UPGRADE_INI       ${LANG} "Se crear� una copia de la configuraci�n actual en ${APP_NAME}-old.ini."
LangString L10N_UPGRADE_INSTALL   ${LANG} "&Instalar en otra ubicaci�n"
LangString L10N_UPGRADE_UNINSTALL ${LANG} "&Uninstall ${APP_NAME}."
LangString L10N_UPDATE_SECTION    ${LANG} "Comprobar actualizaciones antes de instalar"
LangString L10N_UPDATE_DIALOG     ${LANG} "Una nueva versi�n est� disponible.$\n�Anular instalaci�n y visitar la p�gina de descarga?"
LangString L10N_RUNNING           ${LANG} "${APP_NAME} se est� ejecutando. �Desea cerrarlo?"
LangString L10N_RUNNING_UNINSTALL ${LANG} "Si elige No, ${APP_NAME} ser� eliminado por complento en el pr�ximo arranque."
LangString L10N_SHORTCUT          ${LANG} "Acceso directo en el men� Inicio"
LangString L10N_AUTOSTART         ${LANG} "Iniciar con Windows"

!undef LANG
