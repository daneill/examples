#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/smart_ptr.hpp>

using namespace std;
using namespace boost::asio;
using boost::system::error_code;

class HelloWorldAServer {
public:
	//类的初始化创建，设置io_service，设置1000端口
	HelloWorldAServer(io_service & io_serv)
		: _io_serv(io_serv)
		, _tcp_acceptor(io_serv, 
                        ip::tcp::endpoint(ip::tcp::v4(), PORT))
	{
	}
	
	//start accept
	void start()
	{
		//创建一个tcp的socket
		boost::shared_ptr<ip::tcp::socket> p_socket(
				new ip::tcp::socket(_io_serv));
		//为了保证该socket在整个异步操作期间都是有效的，而
		//且以后所有的客户端连接进来后该socket都是有效地，
		//这里的解决办法是使用一个带计数的智能指针，
		//shared_ptr，并将该指针绑定到回调函数上。
		//该智能指针的生存周期等同于sev的生存周期。 
		
		//开始等待连接（非阻塞）
		_tcp_acceptor.async_accept(*p_socket, 
				boost::bind(&HelloWorldAServer::_on_accept_completed, 
					this, p_socket, _1));
		//async_accept其实就是注册一个回调函数，它会立刻返回
		//触发事件只有一个error_code参数，
		//所以用boost::bind把socket绑定进去
		//		boost::bind(&HelloWorldAServer::_on_accept_completed, 
		//			this, p_socket, _1));
		//_on_accept_completed本来是三个参数的，但是这里bind
		//之后就变成一个参数了，符合async_accept()的要求
        /*
        template <
            typename Protocol1,
            typename SocketService,
            typename AcceptHandler>
        void-or-deduced async_accept(basic_socket< Protocol1, SocketService > & peer,
            AcceptHandler handler,
            typename enable_if< is_convertible< Protocol, Protocol1 >::value >::type *  = 0);

        The function signature of the handler must be:
        void handler(
            const boost::system::error_code& error // Result of operation.
        );
        */
	}
	
	void _on_accept_completed(
			boost::shared_ptr<ip::tcp::socket> p_socket, 
			error_code ec)
	{
		if (ec) {
			return;
		}
		
		//继续异步等待下一个连接
		start();
		
		//显示client端IP
		cout << "[server] get request from ip[" 
            << p_socket->remote_endpoint().address() << "]\n";
		
        const int READ_BUFFER_LENGTH = 10240;
        boost::shared_ptr<vector<char> > p_read_buffer(
                new vector<char>(READ_BUFFER_LENGTH, '\0'));
        p_socket->async_read_some(buffer(*p_read_buffer), 
                boost::bind(&HelloWorldAServer::_on_read_completed, 
                    this, p_socket, p_read_buffer, _1, _2));
        /*
        template<
            typename MutableBufferSequence,
            typename ReadHandler>
        void async_read_some(const MutableBufferSequence & buffers,
                            ReadHandler handler);
        
        The function signature of the handler must be:
        void handler(
            const boost::system::error_code& error, // Result of operation.
            std::size_t bytes_transferred           // Number of bytes read.
        );
        */
    }
    	
	//异步read操作完成后触发_on_read_completed
	void _on_read_completed(
			boost::shared_ptr<ip::tcp::socket> p_socket, 
            boost::shared_ptr<vector<char> > p_read_buffer,
            error_code ec, size_t bytes_transferred)
	{
		if (ec) {
			cout << "Fail to read\n";
            return;
		} else {
            cout << "[server] receive request[" << & (* p_read_buffer)[0] << "]\n";
		}
		//需要保证整个异步发送期间缓冲区的有效性，
		//所以使用了shared_ptr<string>参数
		boost::shared_ptr<string> p_write_buffer(
				new string("hello async world!"));
		//发送信息（非阻塞）
		p_socket->async_write_some(buffer(*p_write_buffer),
				boost::bind(&HelloWorldAServer::_on_write_completed, 
					this, p_write_buffer, _1, _2));
        /*
        template<
            typename ConstBufferSequence,
            typename WriteHandler>
        void async_write_some(const ConstBufferSequence & buffers,
                            WriteHandler handler);
        The function signature of the handler must be:
        void handler(
            const boost::system::error_code& error, // Result of operation.
            std::size_t bytes_transferred           // Number of bytes written.
        );
        */
	}
    
	//异步写操作完成后触发_on_write_completed
	void _on_write_completed(boost::shared_ptr<string> p_write_buffer, 
			error_code ec, size_t bytes_transferred)
	{
		if (ec) {
			cout << "Fail to write\n";
		} else {
            cout << "[server] send response[" << * p_write_buffer << "]\n";
		}
	}
	
private:
	io_service & _io_serv;
	ip::tcp::acceptor _tcp_acceptor;
	
	const static int PORT = 9051;
};

int main(int argc, char * argv[])
{
	//建立io服务器
	io_service io_serv;
	
	HelloWorldAServer server(io_serv);
	
	//开始监听socket的连接; 开始接收远程数据
	server.start();
	
	//开始执行回调函数
	io_serv.run();
	//io_service::run()是一个循环，负责分发异步回调函数
	//只有当所有打异步操作执行完成之后才会返回
	
	return 0;
}
