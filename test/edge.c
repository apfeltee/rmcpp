
/** THIS IS NOT A VALID SOURCE FILE! */
do not try to compile me; // please.

SQLITE_PRIVATE const char **sqlite3CompileOptions(int *pnOpt){
  *pnOpt = sizeof(sqlite3azCompileOpt) / sizeof(sqlite3azCompileOpt[0]);
  return (const char**)sqlite3azCompileOpt;
}

    if( strncmp(zUri+5, "///", 3)==0 ){
      iIn = 7;

int sigpause (int) __asm__ ("" "__xpg_sigpause");

/* where is the issue? */
 for(i = 0; zMsg[i] && zMsg[i] != '\r' && zMsg[i] != '\n'; i++)
    {
    }
    zMsg[i] = 0;
    sqlite3_log(errcode, "os_win.c:%d: (%lu) %s(%s) - %s", iLine, lastErrno, zFunc, zPath, zMsg);
    return errcode;
}
/* most common issues follow */
            if(
                /* these break most parsers: */
                (((zBuf[nLen - 1]) == '/') ||
                /* maybe this? */
                ((zBuf[nLen - 1]) == '\\'))
                /* what now? */
            )
            {
                return 1;
            }
            else if(nLen + 1 < nBuf)
            {
                zBuf[nLen] = '\\';
                /* if this comment is still here, your parser is BROKEN! */
                zBuf[nLen + 1] = '\0';
                return 1;
            }

            (xtype==14) ? 
            '"' /* <-- breaks most parsers */
            :
            '\''  /* <-- as does this */
        ); /* Quote character */
        char *escarg;
        if( bArgList ){

      sqlite3_str_append(&out, "-- ", 3);

      /* If the next character is a digit, this is a floating point
      ** number that begins with ".".  Fall thru into the next case */
      if( nToken==0 ) break;
      if( zRawSql[0]=='?' ){

/* foo' */
        ((void)0);
        sqlite3_str_append(&out, "x'", 2);
        nOut = pVar->n;


SQLITE_PRIVATE void sqlite3DeleteTrigger(sqlite3 *db, Trigger *pTrigger){
...
drop_trigger_cleanup:
  sqlite3SrcListDelete(db, pName);
}

/* this breaks surprisingly many strippers: */
    if( strncmp(zUri+5, "///", 3)==0 ){
      iIn = 7;
      /* The following condition causes URIs with five leading / characters
      * ...
      ** common error, we are told, so we handle it as a special case. */
      if( strncmp(zUri+7, "///", 3)==0 ){ iIn++; }
    }else if( strncmp(zUri+5, "//localhost/", 12)==0 ){
      iIn = 16;

// this /* shouldn't be a problem

"well hey there";

/* bork */// <- this breaks most comment strippers

"badonk";

/* messy */

"looks fine!";

/* if you can read this, then the parser is broken. go fix it! */
