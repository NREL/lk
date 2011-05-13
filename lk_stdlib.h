#ifndef __lk_stdlib_h
#define __lk_stdlib_h

#include "lk_env.h"

namespace lk {

	std::vector<fcall_t> stdlib_basic();
	std::vector<fcall_t> stdlib_string();
	std::vector<fcall_t> stdlib_math();

	bool tex_doc( const std::string &file,
				  const std::string &title,
				  std::vector<fcall_t> lib );


	std::vector< std::string > dir_list( const std::string &dir, const std::string &extlist, bool ret_dirs=false );

	std::vector< std::string > split( const std::string &str, const std::string &delim, bool ret_empty=false, bool ret_delim=false );
	std::string join( const std::vector< std::string > &list, const std::string &delim );

	bool to_integer(const std::string &str, int *x);
	bool to_float(const std::string &str, float *x);
	bool to_double(const std::string &str, double *x);

	std::string to_string( int x, const char *fmt="%d" );
	std::string to_string( double x, const char *fmt="%lg" );

	std::string lower_case( const std::string &in );
	std::string upper_case( const std::string &in );

	size_t replace( std::string &s, const std::string &old_text, const std::string &new_text);

	std::string read_file( const std::string &file );
	bool read_line( FILE *fp, std::string &text, int prealloc = 256 );

	bool file_exists( const char *file );
	bool dir_exists( const char *path );
	bool remove_file( const char *path );
	bool mkdir( const char *path, bool make_full = false);
	std::string path_only( const std::string &path );
	std::string name_only( const std::string &path );
	std::string ext_only( const std::string &path );
	char path_separator();
	std::string get_cwd();
	bool set_cwd( const std::string &path );

	class sync_piped_process
	{
	public:
		sync_piped_process() {  }
		virtual ~sync_piped_process() {  }

		int spawn(const std::string &command, const std::string &workdir="");
		virtual void on_stdout(const std::string &line_text) = 0;
	};


	std::string trim_to_columns(const std::string &str, int numcols);
	std::string format_vl( const std::string &fmt, const std::vector< vardata_t* > &args );
	std::string format(const char *fmt, ...);
	size_t format_vn(char *buffer, int maxlen, const char *fmt, va_list arglist);

};


#endif
