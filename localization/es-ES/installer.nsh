;TrayCD - es-ES localization by Fabrizio Ferri (algernon@gmail.com)
;
;This program is free software: you can redistribute it and/or modify
;it under the terms of the GNU General Public License as published by
;the Free Software Foundation, either version 3 of the License, or
;(at your option) any later version.

!insertmacro MUI_LANGUAGE "Spanish"
!define LANG ${LANG_SPANISH}

LangString L10N_UPGRADE_TITLE     ${LANG} "Ya está instalado"
LangString L10N_UPGRADE_SUBTITLE  ${LANG} "Elige el modo de instalación de ${APP_NAME}."
LangString L10N_UPGRADE_HEADER    ${LANG} "${APP_NAME} ya se encuentra instalado. Selecciona la operación que quieres llevar a cabo y pulsa Siguiente para continuar."
LangString L10N_UPGRADE_UPGRADE   ${LANG} "&Actualiza ${APP_NAME} a la versión ${APP_VERSION}."
LangString L10N_UPGRADE_INI       ${LANG} "Se creará una copia de la configuración actual en ${APP_NAME}-old.ini."
LangString L10N_UPGRADE_INSTALL   ${LANG} "&Instalar en otra ubicación"
LangString L10N_UPGRADE_UNINSTALL ${LANG} "&Desinstalar ${APP_NAME}."
LangString L10N_UPDATE_DIALOG     ${LANG} "Una nueva versión está disponible.$\n¿Anular instalación y visitar la página de descarga?"

!undef LANG
