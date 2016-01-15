#include "../lk_absyn.cpp"
#include "../lk_env.cpp"
#include "../lk_eval.cpp"
#include "../lk_parse.cpp"
#include "../lk_lex.cpp"
#include "../lk_stdlib.cpp"
#include "../lk_invoke.cpp"

int main(int argc, char *argv[])
{
	lk_string html;
	
	printf("generating lk_sys.tex...\n");
	lk::tex_doc( "lk_sys.tex", "Standard System Functions", lk::stdlib_basic() );
	html += lk::html_doc( "Standard System Functions", lk::stdlib_basic() );
	
	printf("generating lk_string.tex...\n");
	lk::tex_doc( "lk_str.tex", "String Functions", lk::stdlib_string() );
	html += lk::html_doc( "String Functions", lk::stdlib_string() );
	
	printf("generating lk_math.tex...\n");
	lk::tex_doc( "lk_math.tex", "Math Functions", lk::stdlib_math() );
	html += lk::html_doc( "Math Functions", lk::stdlib_math() );
	
#ifdef __WX__
	printf("generating lk_wx.tex...\n");
	lk::tex_doc( "lk_wx.tex", "GUI Functions", lk::stdlib_wxui() );
	html += lk::html_doc( "User Interface Functions", lk::stdlib_wxui() );
#endif

	FILE *fp = fopen("lk_doc.html", "w");
	if (fp)
	{
		fprintf(fp, "<html><head><title>LK Reference Documentation</title></head><body><font face=\"Verdana,Arial,Helvetica\">\n" );
		fputs( html.c_str(), fp );
		fprintf(fp, "</font></body></html>\n");
		fclose(fp);
		printf("wrote lk_doc.html\n");		
	}
	
	printf("done.\n");	
	return 0;
}
