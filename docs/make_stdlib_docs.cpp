#include "../lk_absyn.cpp"
#include "../lk_env.cpp"
#include "../lk_eval.cpp"
#include "../lk_parse.cpp"
#include "../lk_lex.cpp"
#include "../lk_stdlib.cpp"

int main(int argc, char *argv[])
{
	printf("generating lk_basic.tex...\n");
	lk::tex_doc( "lk_basic.tex", "Basic and I/O Functions", lk::stdlib_basic() );
	
	printf("generating lk_string.tex...\n");
	lk::tex_doc( "lk_str.tex", "String Functions", lk::stdlib_string() );
	
	printf("generating lk_math.tex...\n");
	lk::tex_doc( "lk_math.tex", "Math Functions", lk::stdlib_math() );
	
#ifdef __WX__
	printf("generating lk_wx.tex...\n");
	lk::tex_doc( "lk_wx.tex", "GUI Functions", lk::stdlib_wxui() );
#endif
	
	printf("done.\n");	
	return 0;
}
