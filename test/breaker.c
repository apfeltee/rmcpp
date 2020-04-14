
/** THIS IS NOT A VALID SOURCE FILE! */
do not try to compile me; // thanks.

// i'm a C++ comment

/* blah */
int this_is_visible = "hey there";

fuck = "i use escapes \"to embed quotes\". /* do not touch me */ scary!";

/* this is a test file for rmcpp - it's ****not**** supposed to make sense. */

/* SHOULD BE REMOVED */
#pragma \
    <stdio.h>\


static const short quantum_count;

/* SHOULD BE REMOVED /* <-- should not confuse the parser */
#pragma quantum_count ( this is\
    probably /* hi im a comment */ \
        not \
      /* should be removed*/  how \
                    /* im also a comment! */this directive \
                        works /* should also be removed */ but idgaf)

struct dumb {
    union this_is_not_how_you_use_unions {
        #if __crapwareltd_cpp__
            int crapware_fix_count;
        #endif
        char somemore;
    } thanks_tell_me_more;
};
const char bighuge[(int /*| 0*/ )2e935] = {255};


/* SHOULD REMAIN */
    #               warning \
your \
               /* inline comment */            stuff\
                                    is\
        broken /* another comment? */

    /* this breaks rmcpp.rb */
    string[] array = queryRepresentation.Trim(new char[]
    {
        '\\' /* test 1 */ "<--- should parse";
        /* this might break the parser - it DEFINITELY breaks rmcpp.rb */
        '//' /* will this break everything? */ "<-- why does this fail";
    }).Split(new char[]
    {
        '\\' /* test 2 */
    });

static const struct dumb hugeness;

char this_is_the_end_of_this_file = true;

blah = '" am i broken?"'; /* am i???? */
poop = 'are "you \"broken"?\"'; /* well? */
        char q = ((xtype==etSQLESCAPE3)?'"':'\'');   /* Quote character */
        ...
        if( isnull ) escarg = (xtype==etSQLESCAPE2 ? "NULL" : "(NULL)");
        /* For %q, %Q, and %w, the precision is the number of byte
        ** ...
        */

    case ' ': /* space */
    case '\f': /* formfeed */
    {
        /* White space is ignored */
        token = tkWS;
        break;
    }
/**
this breaks
**/

"this //also breaks";

"fine so far";

/* if you can read this, something broke! */
