/******************************************************************************

  Robot Toolkit ++ (RTK++)

  Copyright (c) 2007-2013 Shuhui Bu <bushuhui@nwpu.edu.cn>
  http://www.adv-ci.com

  ----------------------------------------------------------------------------

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.

*******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include <string>
#include <iostream>

#include "base/utils/utils.h"
#include "Socket++.h"


namespace pi {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int inet4_addr_str2i(std::string na, uint32_t &nai)
{
    inet_pton(AF_INET, na.data(), &nai);
    return 0;
}

int inet4_addr_i2str(uint32_t nai, std::string &na)
{
    char    buf[INET_ADDRSTRLEN+1];

    inet_ntop(AF_INET, &nai, buf, INET_ADDRSTRLEN);
    na = buf;

    return 0;
}

int inet4_addr_ni2hi(uint32_t ni, uint32_t &hi)
{
    hi = ntohl(ni);

    return 0;
}

int inet4_addr_hi2ni(uint32_t hi, uint32_t &ni)
{
    ni = htonl(hi);

    return 0;
}

int inet4_port_n2h(uint16_t np, uint16_t &hp)
{
    hp = ntohs(np);

    return 0;
}

int inet4_port_h2n(uint16_t hp, uint16_t &np)
{
    np = htons(hp);

    return 0;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class RSocket_PrivateData
{
public:
    RSocket_PrivateData() {
        memset(&m_addr, 0, sizeof(sockaddr_in));
        memset(&m_addrClient, 0, sizeof(sockaddr_in));
    }

    ~RSocket_PrivateData() {
        memset(&m_addr, 0, sizeof(sockaddr_in));
        memset(&m_addrClient, 0, sizeof(sockaddr_in));
    }

public:
    sockaddr_in     m_addr;
    sockaddr_in     m_addrClient;
};



////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

RSocket::RSocket() : m_sock ( -1 )
{
    RSocket_PrivateData *pd = new RSocket_PrivateData;
    m_priData = pd;

    m_socketType = SOCKET_TCP;
    m_server = 0;

    m_maxConnections = 500;
    m_maxHostname    = 1024;
}

RSocket::~RSocket()
{
    if ( isOpened() )
        ::close(m_sock);

    RSocket_PrivateData *pd = (RSocket_PrivateData*) m_priData;
    delete pd;
    m_priData = NULL;
}

int RSocket::create(void)
{
    if( m_socketType == SOCKET_TCP )
        m_sock = socket(AF_INET, SOCK_STREAM, 0);
    else
        m_sock = socket(AF_INET, SOCK_DGRAM, 0);

    if ( ! isOpened() )
        return -1;

    // set SO_REUSEADDR on
    if( m_socketType == SOCKET_TCP )  {
        int on = 1;
        if ( setsockopt(m_sock, SOL_SOCKET,
                        SO_REUSEADDR, (const char*) &on, sizeof (on)) == -1 )
            return -2;
    }

    return 0;
}

int RSocket::close(void)
{
    if ( isOpened() )
        ::close(m_sock);

    m_sock = -1;

    return 0;
}

int RSocket::bind(int port)
{
    RSocket_PrivateData *pd = (RSocket_PrivateData*) m_priData;

    if ( !isOpened() ) {
        return -1;
    }

    pd->m_addr.sin_family       = AF_INET;
    pd->m_addr.sin_addr.s_addr  = htonl(INADDR_ANY);
    pd->m_addr.sin_port         = htons(port);

    int ret = ::bind(m_sock, (struct sockaddr*) &(pd->m_addr), sizeof(pd->m_addr));

    if ( ret == -1 ) {
        return -2;
    }

    return 0;
}


int RSocket::listen(void)
{
    if ( !isOpened() ) {
        return -1;
    }

    int listen_return = ::listen(m_sock, m_maxConnections);

    if ( listen_return == -1 ) {
        return -2;
    }

    return 0;
}


int RSocket::accept(RSocket& new_socket)
{
    RSocket_PrivateData *pd = (RSocket_PrivateData*) new_socket.m_priData;

    int addr_length = sizeof(pd->m_addrClient);
    new_socket.m_sock = ::accept(m_sock,
                                 (sockaddr*) &(pd->m_addrClient),
                                 (socklen_t*) &addr_length);

    if ( new_socket.m_sock <= 0 )
        return -1;
    else
        return 0;
}



int RSocket::send(uint8_t *dat, int len)
{
    if( m_socketType == SOCKET_TCP ) {
        return ::send(m_sock, dat, len, MSG_NOSIGNAL);
    } else {
        RSocket_PrivateData *pd = (RSocket_PrivateData*) m_priData;

        if( m_server ) {
            return ::sendto(m_sock, dat, len, 0,
                          (struct sockaddr *) &(pd->m_addrClient), sizeof(pd->m_addrClient));
        } else {
            return ::sendto(m_sock, dat, len, 0,
                          (struct sockaddr *) &(pd->m_addr), sizeof(pd->m_addr));
        }
    }
}

int RSocket::send (std::string &s)
{
    return send((uint8_t*)s.c_str(), s.size());
}

int RSocket::send(RDataStream &ds)
{
    return send(ds.data(), ds.size());
}


int RSocket::recv(uint8_t *dat, int len)
{
    RSocket_PrivateData *pd = (RSocket_PrivateData*) m_priData;

    if( m_socketType == SOCKET_TCP )
        return ::recv(m_sock, dat, len, 0);
    else {
        if( m_server ) {
            socklen_t addrLen = sizeof(pd->m_addrClient);
            return ::recvfrom(m_sock, dat, len, 0,
                              (struct sockaddr *) &(pd->m_addrClient), &addrLen);
        } else {
            return ::recvfrom(m_sock, dat, len, 0, NULL, NULL);
        }
    }
}

int RSocket::recv(std::string& s, int maxLen)
{
    char *buf;
    int status = 0;

    buf = new char[maxLen + 1];
    memset(buf, 0, maxLen + 1);
    s = "";

    status = recv((uint8_t*) buf, maxLen);
    if ( status > 0 ) s = buf;

    delete [] buf;
    
    return status;
}

int RSocket::recv(RDataStream &ds)
{
    ru8     buf[32];
    ru32    header_len, ds_magic, ds_ver, ds_size, ds_size2;
    int     ret;

    // read header
    header_len = 2*sizeof(ru32);
    ret = recv_until(buf, header_len);
    if( ret < header_len ) return -1;

    // get magic, ver, size
    datastream_get_header(buf, ds_magic, ds_ver);
    ds_size = datastream_get_length(buf);

    // resize DataStream
    ds.setHeader(ds_magic, ds_ver);
    ds.resize(ds_size);

    // read contents
    ds_size2 = ds_size - header_len;
    ret = recv_until(ds.data()+header_len, ds_size2);
    if( ret < ds_size2 ) return -1;

    // rewind position index
    ds.rewind();

    return 0;
}

int RSocket::recv_until(uint8_t *dat, int len)
{
    uint8_t     *p;
    int         ret, read, readed = 0;

    p    = dat;
    read = len;

    while(1) {
        ret = recv(p, read);
        if( ret < 0 ) return ret;

        readed += ret;
        p      += ret;

        if( readed >= len ) return readed;

        read = len - readed;
    }

    return -1;
}


int RSocket::connect(std::string host, int port)
{
    RSocket_PrivateData *pd = (RSocket_PrivateData*) m_priData;

    if ( ! isOpened() ) return -1;

    pd->m_addr.sin_family = AF_INET;
    pd->m_addr.sin_port   = htons(port);

    int status = inet_pton(AF_INET, host.c_str(), &(pd->m_addr.sin_addr));

    if ( errno == EAFNOSUPPORT )
        return -2;

    if( m_socketType == SOCKET_TCP ) {
        status = ::connect(m_sock, (sockaddr *) &(pd->m_addr), sizeof(pd->m_addr) );

        if ( status == 0 )
            return 0;
        else
            return -3;
    } else {
        return 0;
    }

}

int RSocket::connect(uint32_t addr, int port)
{
    RSocket_PrivateData *pd = (RSocket_PrivateData*) m_priData;

    if ( ! isOpened() ) return -1;

    pd->m_addr.sin_family       = AF_INET;
    pd->m_addr.sin_port         = htons(port);
    pd->m_addr.sin_addr.s_addr  = addr;

    if( m_socketType == SOCKET_TCP ) {
        int status = ::connect(m_sock, (sockaddr *) &(pd->m_addr), sizeof(pd->m_addr) );

        if ( status == 0 )
            return 0;
        else
            return -3;
    } else {
        return 0;
    }
}

int RSocket::startServer(int port, RSocketType t)
{
    m_socketType = t;
    m_server = 1;

    if ( 0 != create() ) {
        return -1;
    }

    if ( 0 != bind (port) ) {
        return -2;
    }

    if( m_socketType == SOCKET_TCP ) {
        if ( 0 != listen() ) return -3;
    }

    return 0;
}

int RSocket::startClient(std::string host, int port, RSocketType t)
{
    m_socketType = t;
    m_server = 0;

    if ( 0 != create() ) {
        return -1;
    }

    if ( 0 != connect(host, port) ) {
        return -2;
    }

    return 0;
}

int RSocket::startClient(uint32_t addr, int port, RSocketType t)
{
    m_socketType = t;

    if ( 0 != create() ) {
        return -1;
    }

    if ( 0 != connect(addr, port) ) {
        return -2;
    }

    return 0;
}


int RSocket::getMyAddress(RSocketAddress &a)
{
    RSocket_PrivateData *pd = (RSocket_PrivateData*) m_priData;

    a.port      = ntohs(pd->m_addr.sin_port);
    a.addr_inet = ntohl(pd->m_addr.sin_addr.s_addr);
    inet_ntop(AF_INET, &(pd->m_addr.sin_addr),
              a.addr_str, INET_ADDRSTRLEN);
    a.type = m_socketType;

    return 0;
}

int RSocket::getClientAddress(RSocketAddress &a)
{
    RSocket_PrivateData *pd = (RSocket_PrivateData*) m_priData;

    a.port      = ntohs(pd->m_addrClient.sin_port);
    a.addr_inet = ntohl(pd->m_addrClient.sin_addr.s_addr);
    inet_ntop(AF_INET, &(pd->m_addrClient.sin_addr),
              a.addr_str, INET_ADDRSTRLEN);
    a.type = m_socketType;

    return 0;
}

int RSocket::setNonBlocking(int nb)
{
    int opts;

    opts = fcntl(m_sock, F_GETFL);

    if ( opts < 0 ) {
        return -1;
    }

    if ( nb )
        opts = opts | O_NONBLOCK;
    else
        opts = opts & ~O_NONBLOCK;

    fcntl(m_sock, F_SETFL, opts);

    return 0;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int NetTransfer_UDP::thread_func(void *arg)
{
    int     st = 0, ret;

    ru8     *buf, *data_buf = NULL;
    ru32    buf_len, data_len;
    ru32    msgid;
    ru32    *pi;
    ru8     magic[4], num_buf[8];

    int     mi = 0, ni = 0, ci = 0,
            ri = 0;

    // copy magic number
    pi = (ru32*) magic;
    *pi = m_magicNum;                           // FIXME: better way?

    // create receiving buffer
    buf_len = m_bufLen;
    buf = new ru8[buf_len];

    // loop until stop
    while( getAlive() ) {
        ret = m_socket.recv(buf, buf_len);
        if( ret < 0 ) break;
        if( ret == 0 ) tm_sleep(2);

        //dbg_pt("buf_len = %d, ret = %d\n", buf_len, ret);

        ri = 0;

ST_BEGIN:
        if( st == 0 ) {
            // check magic number
            while(ri < ret ) {
                if( buf[ri++] != magic[mi++] ) mi = 0;

                if( mi >= 4 ) {
                    ni = 0;
                    st = 1;
                    goto ST_BEGIN;
                }
            }
        } else if ( st == 1 ) {
            // get data length & msgid
            while( ri < ret ) {
                num_buf[ni++] = buf[ri++];

                if( ni >= 8 ) {
                    pi = (ru32*) num_buf;
                    data_len = *pi;
                    msgid    = *(pi+1);

                    //dbg_pt("data_len = %d, msgid = %d\n", data_len, msgid);

                    if( data_len == 0 ) {
                        st = 0;
                        goto ST_BEGIN;
                    }

                    data_buf = new ru8[data_len];

                    ci = 0;
                    st = 2;
                    goto ST_BEGIN;
                }
            }
        } else if ( st == 2 ) {
            // copy payload
            while( ri < ret ) {
                data_buf[ci++] = buf[ri++];

                if( ci >= data_len ) {
                    // call user recv function
                    recv(data_buf, data_len, msgid);

                    // free buffer
                    delete [] data_buf;
                    data_buf = NULL;

                    // reset state
                    st = 0;
                    mi = 0;
                    goto ST_BEGIN;
                }
            }
        }
    }

    delete [] buf;
    if( data_buf != NULL ) delete [] data_buf;

    return 0;
}


int NetTransfer_UDP::open(int isServer, int port, std::string addr)
{
    m_isServer = isServer;
    m_addr = addr;
    m_port = port;

    if( m_isServer ) {
        // start UDP server
        if( 0 != m_socket.startServer(m_port, SOCKET_UDP) ) {
            m_isConnected = 0;
            return -1;
        }

        // start thread
        start();
    } else {
        if( 0 != m_socket.startClient(m_addr, m_port, SOCKET_UDP) ) {
            m_isConnected = 0;
            return -1;
        }
    }

    m_isConnected = 1;

    return 0;
}

int NetTransfer_UDP::close(void)
{
    if( !m_isConnected ) return -1;

    if( m_isServer ) {
        setAlive(0);
        wait(20);
        kill();
    }

    m_socket.close();
    m_isConnected = 0;

    return 0;
}

int NetTransfer_UDP::send(ru8 *dat, ru32 len, ru32 msgid)
{
    ru8     msgHeader[12];
    int     ret;

    ru8     *p;
    int     nsend, n;

    if( !m_isConnected ) return -1;
    if( len == 0 ) return 0;

    // send header
    // FIXME: better way?
    memcpy(msgHeader,   &m_magicNum, sizeof(ru32));
    memcpy(msgHeader+4, &len,        sizeof(ru32));
    memcpy(msgHeader+8, &msgid,      sizeof(ru32));

    ret = m_socket.send(msgHeader, 12);
    if( ret != 12 ) return ret;

    // send contents
    nsend = 0;

    while(1) {
        if( nsend >= len ) break;

        p = dat + nsend;
        n = len - nsend;
        if( n > m_bufLen ) n = m_bufLen;

        ret = m_socket.send(p, n);
        //dbg_pt("m_socket.send n = %d, ret = %d\n", n, ret);
        if( ret < 0 ) return ret;

        nsend += ret;
    }

    return nsend;
}


} // end of namespace pi

