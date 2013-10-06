#include <iostream>
#include <fstream>
#include <iterator>
#include <string>
#include <list>
#include <algorithm>
#include <functional>
#include <clocale>
#include <cctype>

#ifdef NDEBUG
#include <windows.h>
#endif

using namespace std;

typedef list<string> commands;

void add_double_quote(string& st)
{
	if (*st.begin() != '"') {
		st.insert(0, 1, '"');
	}
	if (*st.rbegin() != '"') {
		st.push_back('"');
	}
}

void remove_double_quote(string& st)
{
	remove_if(st.begin(), st.end(),
			  bind1st(equal_to<string::value_type>(), '"'));
}

bool load(const char* filename, const string& cmd, commands& cmdlist, string* replace)
{
	ifstream file(filename);
	string buf;
	bool find = false;
	
	if (cmd.empty()) {
		while (getline(file, buf)) {
			cmdlist.push_back(buf);
		}
		return find;
	}
	while (getline(file, buf)) {
		if (buf.length() < cmd.length() ||
			buf.compare(buf.length() - cmd.length(), cmd.length(), cmd) != 0 ||
			(buf.length() > cmd.length() && 
				(! isspace(buf[buf.length() - cmd.length() - 1]) &&
					buf[buf.length() - cmd.length() - 1] != '"'))) {
			cmdlist.push_back(buf);
		} else {
#ifndef NDEBUG
			cerr << "'" << buf << "'" << endl;
#endif
			if (replace) {
				replace->swap(buf);
			}
			find = true;
		}
	}
	return find;
}

void save(const char* filename, const commands& cmdlist)
{
	ofstream file(filename);
#ifndef NDEBUG
	copy(cmdlist.begin(), cmdlist.end(), ostream_iterator<string>(cerr, "\n"));
#endif
	copy(cmdlist.begin(), cmdlist.end(), ostream_iterator<string>(file, "\n"));
}

void ensure_header(commands& cmdlist)
{
	if (cmdlist.empty() || cmdlist.begin()->compare(0, 3, "afx")) {
		cmdlist.insert(cmdlist.begin(), "afx afxbkmk");
	}
}

void add_item(commands& cmdlist, const string& line)
{
	commands::iterator i = cmdlist.begin();
	i ++;
	cmdlist.insert(i, line);
}

const char *basename(const char *name, int n)
{
	const char *p;
	
	if (n < 1 || ! name || ! *name) return name;
	p = name + strlen(name) - 1;
#ifdef NDEBUG
	for (; p > name; p = CharPrevA(name, p)) {
		if (*p == '\\' && -- n == 0) return CharNextA(p);
	}
#else
	for (; p > name; p --) {
		if (*p == '\\' && -- n == 0) return p + 1;
	}
#endif
	return p;
}

int islonum(int c) { return islower(c) || isdigit(c); }
int isupnum(int c) { return isupper(c) || isdigit(c); }

class sequence {
	typedef unsigned int bits;
	bits m_flag: 3;
	bits m_sequence;
public:
	sequence() : m_flag(0), m_sequence(0) {}
	bool is_enabled() const { return m_flag != 0; }
	void set_number0() { m_sequence = (m_sequence << 3) | 1; m_flag |= 1; }
	void set_number1() { m_sequence = (m_sequence << 3) | 5; m_flag |= 1; }
	void set_lower()   { m_sequence = (m_sequence << 3) | 2; m_flag |= 2; }
	void set_upper()   { m_sequence = (m_sequence << 3) | 6; m_flag |= 4; }
	void reverse()
	{
		bits seq = m_sequence, rev = 0;
		for (; seq != 0; seq >>= 3) {
			rev = (rev << 3) | (seq & 7);
		}
		m_sequence = rev;
	}
	char get_sequence_char() const
	{
		return " 0a0A0a0"[m_flag];
	}
	void apply(commands&) const;
};

void sequence::apply(commands& cmdlist) const
{
	if (! is_enabled()) return;
	static int (*range[])(int) = {
		0, isdigit, islower, islonum, isupper, isupnum, isalpha, isalnum,
	};
	int (*isrange)(int) = range[m_flag];
	static char *begins = " 0a  1A";
	static char *ends = " 9z  9Z";
	char c, end;
	bits seq = m_sequence;
	c = begins[seq & 7];
	end = ends[seq & 7];
	seq >>= 3;
	for (commands::iterator i = cmdlist.begin(); i != cmdlist.end(); ++ i) {
		if ((*i)[0] != '"') continue; // ignore no label
		if ((*i)[2] == ' ' && isrange((*i)[1])) {
			(*i)[1] = c;
			if (c == end) {
				if (seq == 0) seq = m_sequence;
				c = begins[seq & 7];
				end = ends[seq & 7];
				seq >>= 3;
			} else {
				c ++;
			}
		}
	}
}

struct options {
	unsigned int m_delete: 1;
	unsigned int m_move : 2;
	sequence m_sequence;
	const char *m_filename;
	string m_dirname, m_label;
	unsigned int m_execute;
	options(int argc, char *argv[]);
};

options::options(int argc, char *argv[])
	: m_delete(0), m_move(0), m_sequence(), m_filename(0),
	  m_dirname(), m_label(), m_execute(0)
{
	int params = 0, base = 0;
	bool label_with_path = false, exact_label = false;
	const char *dirname = "";
	
	for (int i = 1; i < argc; i ++) {
		if (argv[i][0] == '-') {
			for (char *a = argv[i] + 1; *a; a ++) {
				if (*a == 'd') {
					m_delete = 1;
					m_move = 0;
				}
				else if (*a == 'm') {
					m_delete = 0;
					m_move = 1;
				}
				else if (*a == 'M') {
					m_delete = 0;
					m_move = 2;
				}
				else if (*a == 'l') {
					label_with_path = true;
				}
				else if (*a == 'L') {
					exact_label = true;
				}
				else if (*a == 'n') {
					m_sequence.set_number0();
				}
				else if (*a == 'N') {
					m_sequence.set_number1();
				}
				else if (*a == 'a') {
					m_sequence.set_lower();
				}
				else if (*a == 'A') {
					m_sequence.set_upper();
				}
				else if (*a == 'e') {
					m_execute = 1;
				}
				else if (*a >= '1' && *a <= '9') {
					base = *a - '0';
				}
			}
		}
		else {
			switch (params ++) {
			case 0:
				m_filename = argv[i];
				break;
			case 1:
				dirname = argv[i];
				break;
			case 2:
				m_label = argv[i];
				break;
			default:
				if (! isspace(*argv[i])) m_label.append(" ");
				m_label.append(argv[i]);
				break;
			}
		}
	}
	m_sequence.reverse();
	if (m_label.empty()) {
		m_label = basename(dirname, base);
		exact_label = false;
	}
	else if (label_with_path) {
		m_label.append(" ");
		m_label.append(basename(dirname, base));
	}
	remove_double_quote(m_label);
	if (! exact_label && m_sequence.is_enabled()) {
		m_label.insert(0, "  ");
		m_label[0] = m_sequence.get_sequence_char();
	}
	add_double_quote(m_label);
	m_dirname = dirname;
	if (! m_dirname.empty()) {
		remove_double_quote(m_dirname);
		add_double_quote(m_dirname);
		if (m_execute == 1) {
			m_dirname.insert(0, "&EXEC ");
		} else {
			m_dirname.insert(0, "&CD ");
		}
	}
}

#ifndef NDEBUG
int main(int argc, char *argv[])
#else
#define argc __argc
#define argv __argv
int WINAPI WinMain(
	HINSTANCE hInstance, 
	HINSTANCE hPreInst, 
	LPSTR lpszCmdLine, 
	int nCmdShow)
#endif
{
	setlocale(LC_ALL, "japanese");
	if (argc < 3) {
		static const char* err =
			"usage: afxbkmk [options] menu_file [folder [label]]";
#ifndef NDEBUG
		cerr << err << endl;
#else
		MessageBox(NULL, err, "afxbkmk", MB_OK);
#endif
		return 0;
	}
	options opts(argc, argv);
	commands cmdlist;
	if (! opts.m_sequence.is_enabled() && opts.m_dirname.empty()) {
		return 0;
	}
	bool find = load(opts.m_filename, opts.m_dirname, cmdlist, (opts.m_move != 2) ? NULL : &opts.m_label);
	ensure_header(cmdlist);
	if (opts.m_move == 2 && find) {
		opts.m_dirname.clear();
		opts.m_sequence.apply(cmdlist);
		add_item(cmdlist, opts.m_label);
		opts.m_label.clear();
	} else {
		if (! opts.m_delete && (! find || opts.m_move) && ! opts.m_dirname.empty()) {
			add_item(cmdlist, opts.m_label + "\t" + opts.m_dirname);
		}
		opts.m_dirname.clear();
		opts.m_label.clear();
		opts.m_sequence.apply(cmdlist);
	}
	save(opts.m_filename, cmdlist);
	return 0;
}
