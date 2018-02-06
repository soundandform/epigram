#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <algorithm>
#include <cstdlib>
#include <deque>
#include <list>
#include <set>

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include "EpilogCliServer.hpp"

using namespace std;

using boost::asio::ip::tcp;

typedef boost::shared_ptr<tcp::socket> socket_ptr;

#include "JdCore.hpp"
#include "EpilogLib.hpp"


void RunSession (socket_ptr sock, EpilogMsgQueue * i_queue)
{
	u32 dataRecieved = 0, totalLength = 0;

	try
	{
		char data [c_epilogMaxTcpLength];
		char *ptr = data;
		
		for (;;)
		{
			boost::system::error_code error;
            
            u32 readLength = c_epilogMaxTcpLength - totalLength;
			size_t length = sock->read_some (boost::asio::buffer (ptr, readLength), error);
            
			totalLength += length;
			ptr += length;

			dataRecieved += length;
			
//						cout << "tl: " << totalLength << endl;
			
			while (totalLength >= sizeof (EpilogQueuedMsgHeader))
			{
				auto msg = (EpilogMsg *) data;
			
				u32 totalNeededPacketSize = sizeof (EpilogQueuedMsgHeader) + msg->msgSize;

//				cout << "needed: " << totalNeededPacketSize << " have: " << totalLength << endl;
				if (totalLength >= totalNeededPacketSize)
				{
					d_jdAssert (totalNeededPacketSize <= sizeof (EpilogMsg), "message overflow");
					
					i_queue->QueueMessage (* msg);
					
					totalLength -= totalNeededPacketSize;
					ptr -= totalNeededPacketSize;
					
					if (totalLength > 0)
						memmove (data, & data [totalNeededPacketSize], totalLength);
				}
                else break;
			}

			if (error == boost::asio::error::eof) 
				break; // Connection closed cleanly by peer.
			else if (error) 
				throw boost::system::system_error(error); // Some other error.
		}
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception in thread: " << e.what() << "\n";
	}
}


void StartServer (boost::asio::io_service& io_service, short port, EpilogMsgQueue *i_queue) //, NCurses &i_curses)
{
	tcp::endpoint endPoint (tcp::v4(), port);
	
	tcp::acceptor a (io_service, endPoint);
	for (;;)
	{
		socket_ptr socket (new tcp::socket (io_service));
		
//		cout << socket->local_endpoint().address().to_v6().to_string() << endl;  no go
		
		a.accept (*socket);

		boost::thread t (boost::bind (RunSession, socket, i_queue));
	}
}


class TcpServer
{
	public:
		TcpServer (EpilogMsgQueue *i_queue, uint16_t i_portNumber)
		:
		m_queue			(i_queue),
		m_portNumber	(i_portNumber) { }
	
		void operator () ()
		{
			boost::asio::io_service io_service;
			StartServer (io_service, m_portNumber, m_queue);
		}
		
	EpilogMsgQueue *			m_queue;
	uint16_t					m_portNumber;
};



#include <boost/program_options.hpp>

/*
string Trim (string i_string)
{
	while (i_string.find (" ") == 0)
		i_string = i_string.substr (1);
	while (i_string.rfind (" ") == 0)
		i_string = i_string.substr (1);

}
*/

vector <string> SplitCategories (std::string i_categories)
{
	vector <string> split;
	size_t pos;
	while ((pos = i_categories.rfind (",")) != string::npos)
	{
		split.push_back (i_categories.substr (pos+1));
		i_categories = i_categories.substr(0, pos);
	}
	split.push_back (i_categories);
	
	return split;
}


template <typename Setter, typename T>
bool ConfigureCategory (vector <EpilogCategory> &io_category, const vector<string> &i_categories, const T &i_value)
{
	for (int i = 0; i < i_categories.size(); ++i)
	{
		string categoryName = i_categories [i];
		
		bool foundCategory = false;
		for (auto c : io_category)
		{
			if (c.name == categoryName)
				foundCategory = true;
		}
		
		if (! foundCategory)
		{
			EpilogCategory category = { categoryName, false, 0, e_epilogColor_black };
			io_category.push_back (category);
		}
		
//		bool found = false;
		for (int j = 0; j < io_category.size(); ++j)
		{
			if (io_category [j].name == categoryName || categoryName == "*")
			{
				Setter::Set (io_category[j], i_value);
//				found = true;
			}
		}
		
//		if (! found)
//		{
//			cout << "not a valid category: "<< categoryName << endl;
//			return false;
//		}
	}
	
	return true;
}


int main(int argc, char* argv[])
{
	cout << "built: " << __DATE__ " " << __TIME__ << endl;
	std::map <string, string> category;

	namespace po = boost::program_options;

	po::options_description desc ("Allowed options");
	desc.add_options() 
		("help", "produce help message")
		("curses", "use curses")
		("port", po::value <uint16_t>(), "set the port #")
		("level", po::value <std::string>(), "filter out priorities below this value. options: tedious, detail, normal")
		("hide", po::value <std::string>(), "filter out these categories; comma separate multiple categories")
		("show", po::value <std::string>(), "show only these categories; comma separate multiple categories")
		("red", po::value <std::string>(), "display these categories in red; comma separate multiple categories")
		("green", po::value <std::string>(), "display these categories in green; comma separate multiple categories")
		("blue", po::value <std::string>(), "display these categories in blue; comma separate multiple categories")
		("cyan", po::value <std::string>(), "display these categories in cyan; comma separate multiple categories")
		("purple", po::value <std::string>(), "display these categories in purple; comma separate multiple categories")
		("yellow", po::value <std::string>(), "display these categories in yellow; comma separate multiple categories")
		("log", "save a time-ordered log file");
	
	po::variables_map vm;
	po::store (po::parse_command_line (argc, argv, desc), vm);
	po::notify (vm);    
	
	if (vm.count("help"))
	{
		cout << desc << "\n";
		return 1;
	}
	
	bool useCurses = vm.count ("curses");
	
	int port = 9999; // c_epigramLogDefaultPort;
	
	if (vm.count ("port"))
		port = vm["port"].as <uint16_t>();
	
	
	vector <EpilogCategory> categories;
	map <string, EpilogCategory *> catagoryMap;
	
	/*
	vector <string> filter;
	if (vm.count ("filter"))
	{
		string category = vm["filter"].as <string> ();
		
		cout << "showing: " << categories [category] << endl;
		
		category = categories [category];
		size_t pos;
		while ((pos = category.rfind (" ")) != string::npos)
		{
			filter.push_back (category.substr (pos+1));
			category = category.substr(0, pos);
		}
		filter.push_back (category);
	}
	*/

//	int j = 0;
//	for (auto i = category.begin(), e = category.end(); i != e; ++i)
//	{
//		string cat = i->first;
//		string classes = i->second;
//
//		classes = " " + classes;
//		
//		EpilogCategory * category = new EpilogCategory { cat, false, c_epilogClassification_normal, e_epilogColor_black };
////		cout << "Registered Categories: '" << cat << "'\n";
//		
////s		catagoryMap.push_back (category);
//		size_t pos;
//		while ((pos = classes.rfind (" ")) != string::npos)
//		{
//			string clas = classes.substr (pos+1);
//			
//			//cout << "class='" << clas << "' " << endl;
//			
////			categoryMap
////			classToCategoryIndex [clas] = j+1;
//			classes = classes.substr(0, pos);
//		}
//
//		++j;
//	}
//
//	
	
	const char *colors [] = { "red", "green", "blue", "cyan", "purple", "yellow" };
	for (int i = 0; i < Jd::SizeOfArray (colors); ++i)
	{
		if (vm.count (colors[i]))
		{
			string commaSeparated = vm [colors[i]].as <string> ();
			
			struct ColorSetter
			{
				static void Set (EpilogCategory &i_category, const EEpilogColorIndex i_color)
				{
					cout << "set: " << i_category.name << " to color: " << i_color << endl;
					i_category.color = i_color;
				}
			};

			if (! ConfigureCategory <ColorSetter> (categories, SplitCategories (commaSeparated), (EEpilogColorIndex) (i+1)))
					return -1;
		}
	}

	if (vm.count ("hide"))
	{
		string commaSeparated = vm ["hide"].as <string> ();
		
		struct Disabler
		{
			static void Set (EpilogCategory &i_category, const bool i_filter)
			{
				i_category.filter = i_filter;
			}
		};
		
		if (! ConfigureCategory <Disabler> (categories, SplitCategories (commaSeparated), true))
			return -1;
	}
	
	if (vm.count ("show"))
	{
		string commaSeparated = vm ["show"].as <string> ();
		
		struct Disabler
		{
			static void Set (EpilogCategory &i_category, const bool i_filter)
			{
				i_category.filter = i_filter;
			}
		};

		ConfigureCategory <Disabler> (categories, { "*" }, true);
		
		if (! ConfigureCategory <Disabler> (categories, SplitCategories (commaSeparated), false))
			return -1;
	}

	

	string level = "tedious";
	if (vm.count ("level")) level = vm ["level"].as <string> ();

	uint8_t intLevel = 0;
	if (level == "tedious") intLevel = c_epilogClassification_tedious;
	if (level == "detail") intLevel = c_epilogClassification_detail;
	if (level == "normal") intLevel = c_epilogClassification_normal;
	
	if (intLevel == 0)
	{
		cout << "\ninvalid priority level. options: tedious, detail, normal" << endl;
		return -1;
	}
	
	const char * const c_epilogClassifications [8] = { "realtime", "tedious", "detail", "reserved",  "reserved", "reserved", "reserved", "normal" };
	
	cout << "level: " << c_epilogClassifications [intLevel] << endl;

	struct Disabler
	{
		static void Set (EpilogCategory &i_category, const uint8_t i_level)
		{
			i_category.level = i_level;
		}
	};
	
	vector <string> splat; splat.push_back("*");
	if (! ConfigureCategory <Disabler> (categories, splat, intLevel))
		return -1;
	
    using boost::asio::ip::tcp;

	try
	{
		EpilogMsgQueue queue (1024);
		
		TcpServer server (&queue, port);
		boost::thread t (server);
		
		if (! useCurses)
		{
			EpilogCliServer epg (&queue, false, categories); //, classToCategoryIndex);

			boost::asio::io_service io_service;
			tcp::resolver resolver(io_service);
			
//			tcp::resolver::query query (boost::asio::ip::host_name (), to_string (port));  // this doesn't work anymore!? IPv6?
			
			
			tcp::resolver::query query ("localhost", to_string (port) //,
//										boost::asio::ip::resolver_query_base::address_configured |
//										boost::asio::ip::resolver_query_base::all_matching |
//										boost::asio::ip::resolver_query_base::v4_mapped
										);

			tcp::resolver::iterator iter = resolver.resolve (query);
			tcp::resolver::iterator end;
			
			tcp::endpoint ep;
			while (iter != end)
			{
				ep = *iter++;
				if (ep.address().is_v4())
				{
					auto address = ep.address().to_string ();
					
					if (address != "127.0.0.1")
						cout << "epilogger (" << address << ", " << port << ")";
				}
			}
			
			cout << "--------------------------------\n";
			
			epg.Run();
		}
//		else
//		{
//			NCurses damn;
//			damn.Display(0, 0, ep.address().to_string());
//			damn.Refresh();
//			
//			CEpilogServer server (&queue, damn, vm.count ("log"));
//			server.Run();
//		}
	}

	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}



#if 0

int main()
{	FIELD *field[3];
	FORM  *my_form;
	int ch;
	
	/* Initialize curses */
	initscr();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);
	
	/* Initialize the fields */
	field[0] = new_field(1, 10, 4, 18, 0, 0);
	field[1] = new_field(1, 10, 6, 18, 0, 0);
	field[2] = NULL;
	
	/* Set field options */
	set_field_back(field[0], A_UNDERLINE); 	/* Print a line for the option 	*/
	field_opts_off(field[0], O_AUTOSKIP);  	/* Don't go to next field when this */
	/* Field is filled up 		*/
	set_field_back(field[1], A_UNDERLINE); 
	field_opts_off(field[1], O_AUTOSKIP);
	
	/* Create the form and post it */
	my_form = new_form(field);
	post_form(my_form);
	refresh();
	
	mvprintw(4, 10, "Value 1:");
	mvprintw(6, 10, "Value 2:");
	refresh();
	
	/* Loop through to get user requests */
	while((ch = getch()) != KEY_F(1))
	{	switch(ch)
		{	case KEY_DOWN:
				/* Go to next field */
				form_driver(my_form, REQ_NEXT_FIELD);
				/* Go to the end of the present buffer */
				/* Leaves nicely at the last character */
				form_driver(my_form, REQ_END_LINE);
				break;
			case KEY_UP:
				/* Go to previous field */
				form_driver(my_form, REQ_PREV_FIELD);
				form_driver(my_form, REQ_END_LINE);
				break;
			default:
				/* If this is a normal character, it gets */
				/* Printed				  */	
				form_driver(my_form, ch);
				break;
		}
	}
	
	/* Un post form and free the memory */
	unpost_form(my_form);
	free_form(my_form);
	free_field(field[0]);
	free_field(field[1]); 
	
	endwin();
	return 0;
}
#endif
