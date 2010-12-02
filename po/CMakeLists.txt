SET(LINGUAS ru)
SET(POTFILE "${CNAME}.pot")

ADD_CUSTOM_TARGET(translation ALL DEPENDS)
ADD_CUSTOM_COMMAND(TARGET translation
                   COMMAND xgettext -k_ ../src/*.c -o ${POTFILE})

FOREACH    (LANG ${LINGUAS})
  SET (TRANS "translations/${LANG}.po")

  IF    (EXISTS ${TRANS})
    ADD_CUSTOM_COMMAND(TARGET translation
                       COMMAND msgmerge ${TRANS} ${POTFLE} -o ${LANG}.po
                       COMMAND msgformat -c "${LANG}.po" -o "${LANG}.mo")
  ENDIF (EXISTS ${TRANS})


ENDFOREACH (LANG)
