/*
 * Author: Sven Gothel <sgothel@jausoft.com>, Svenson Han Göthel
 * Copyright (c) 2024 Gothel Software e.K.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <ostream>
#include <fstream>
#include <iomanip>
#include <limits>
#include <regex>

#include <jau/file_util.hpp>
#include <jau/io_util.hpp>
#include <jau/byte_stream.hpp>

#include <jau/debug.hpp>

extern "C" {
    #include <unistd.h>
}

using namespace jau::fractions_i64_literals;
using namespace jau::int_literals;

static const unsigned MaxConnections = 2;
static const std::string HorizonUri = "https://ssd.jpl.nasa.gov/api/horizons_file.api";
static const std::string ObjectIdMark = "OBJECT_ID";
static const std::string ObjectDateMark = "OBJECT_DATE";
static const std::string HorizonCmdTemplate = 
    "!$$SOF\n"
    "COMMAND='"+ObjectIdMark+"'\n"
    "TABLE_TYPE='Vector'\n"
    "CENTER='@010'\n"
    "REF_PLANE='Ecliptic'\n"
    "START_TIME='"+ObjectDateMark+" 00:00:00'\n"
    "STOP_TIME='"+ObjectDateMark+" 00:00:01'\n";
static const std::string ZeroHour = "00:00:00";

static std::string getCommand(const std::string& objectID, const std::string& objectDate) {
    std::string s = HorizonCmdTemplate;
    
    size_t idx = s.find(ObjectIdMark, 0);
    if (idx == std::string::npos) {
        return std::string(); // error
    }
    s.replace(idx, ObjectIdMark.length(), objectID);

    idx = 0;
    while (true) {
        idx = s.find(ObjectDateMark, idx);
        if (idx == std::string::npos) break;

        /* Make the replacement. */
        s.replace(idx, ObjectDateMark.length(), objectDate);

        idx += objectDate.length();
    }
    return s;
}

static std::string toString(unsigned year, unsigned month, unsigned day) {
    // yyyy-mm-dd -> 10 chars
    std::string str;
    str.reserve(10+1);  // incl. EOS
    str.resize(10); // excl. EOS
    
    ::snprintf(&str[0], 10+1, "%4.4u-%2.2u-%2.2u", year, month, day);
    return str;
}

/// @param planetIDx ranges from [1..9]
static unsigned toCBodyID(unsigned planetIdx) {
    return (( planetIdx * 100) + 99)%1000;
}

/// @param planetIDx ranges from [1..9]
static unsigned toBarycenterID(unsigned planetIdx) {
    return ( planetIdx )%10;
}

/// @param planetIDx ranges from [1..9]
static std::string toString(unsigned planetIdx) {
    std::string str;
    str.reserve(3+1);  // incl. EOS
    str.resize(3); // excl. EOS
    
    ::snprintf(&str[0], 3+1, "%3.3u", (( planetIdx * 100) + 99)%1000);
    return str;
}

static std::regex PosPattern( R"(X *= *([\-\+]?\d+\.\d+E[\+\-]\d\d) *Y *= *([\-\+]?\d+\.\d+E[\+\-]\d\d) *Z *= *([\-\+]?\d+\.\d+E[\+\-]\d\d))");
static std::regex VeloPattern( R"(VX *= *([\-\+]?\d+\.\d+E[\+\-]\d\d) *VY *= *([\-\+]?\d+\.\d+E[\+\-]\d\d) *VZ *= *([\-\+]?\d+\.\d+E[\+\-]\d\d))");
static constexpr bool DBG_OUT = false;

static bool getPosVelo(const std::string& data, double pos[/*3*/], double velo[/*3*/]) {
    int matchCount = 0;
    ::memset(&pos[0], 0, sizeof(double)*3);
    ::memset(&velo[0], 0, sizeof(double)*3);
    
    std::smatch match;
    if( std::regex_search(data, match, PosPattern) ) {
        // ssize_t m_strEnd = match.position() + match.length();
        if constexpr ( DBG_OUT ) {
            // std::cerr << "POS: " << data << std::endl;
            std::cerr << "POS: match pos " << match.position() << ", len " << match.length() << ", sz " << match.size() << std::endl;
            for(size_t i=0; i<match.size(); ++i) {
                const std::string& s = match[i];
                std::cerr << "- [" << i << "]: '" << s << "', len " << s.length() << std::endl;
            }
        }
        if( 4 == match.size() ) {
            pos[0] = ::atof(match[1].str().c_str());
            pos[1] = ::atof(match[2].str().c_str());
            pos[2] = ::atof(match[3].str().c_str());
            ++matchCount;
        }
    } else {
        if constexpr ( DBG_OUT ) {
            std::cerr << "POS: n/a" << std::endl;
        }
    }
    if( std::regex_search(data, match, VeloPattern) ) {
        // ssize_t m_strEnd = match.position() + match.length();
        if constexpr ( DBG_OUT ) {
            // std::cerr << "VELO: " << data << std::endl;
            std::cerr << "VELO: match pos " << match.position() << ", len " << match.length() << ", sz " << match.size() << std::endl;
            for(size_t i=0; i<match.size(); ++i) {
                const std::string& s = match[i];
                std::cerr << "- [" << i << "]: '" << s << "', len " << s.length() << std::endl;
            }
        }
        if( 4 == match.size() ) {
            velo[0] = ::atof(match[1].str().c_str());
            velo[1] = ::atof(match[2].str().c_str());
            velo[2] = ::atof(match[3].str().c_str());
            ++matchCount;
        }
    } else {
        if constexpr ( DBG_OUT ) {
            std::cerr << "VELO: n/a" << std::endl;
        }
    }
    return 2 == matchCount;
} 

static std::string readFile(const std::string& path) {
    constexpr size_t read_size = std::size_t(4096);
    std::ifstream stream(path);
    stream.exceptions(std::ios_base::badbit);
    if (not stream) {
        return std::string();
    }    
    auto out = std::string();
    auto buf = std::string(read_size, '\0');
    while (stream.read(& buf[0], read_size)) {
        out.append(buf, 0, stream.gcount());
    }
    out.append(buf, 0, stream.gcount());
    return out;
}

static const std::string HttpBoundarySep = "--";
static const std::string HttpBoundary = "affedeadbeaf";
static const std::string CRLF = "\r\n";

static int64_t toUnixSeconds(const std::string& ymd_timestr) noexcept {
    struct std::tm tm_0; 
    ::memset(&tm_0, 0, sizeof(tm_0));
    ::strptime(ymd_timestr.c_str(), "%Y-%m-%d %H:%M:%S", &tm_0);
    std::time_t t1 = ::timegm (&tm_0);
    return static_cast<int64_t>(t1);
}

struct CBodyData {
    // Horizon celestial body ID `pidx * 100 + 99`, i.e. pidx=1 for Mercury -> id=199
    unsigned id;
    // Position on the ecliptical plane w/ units in [km] 
    double position[3];
    // Velocity vector on the ecliptical plane w/ units in [km/s]
    double velocity[3];
};
struct SolarData {
    /// Timestamp in UTC, format YYYY-MM-DD HH:MM:SS
    std::string time_s;
    /// Seconds since Unix Epoch 1970-01-01T00:00:00.0Z in UTC
    int64_t time_u; 
    std::vector<CBodyData> planets;
};
struct SolarDataSet {
    size_t setCount;
    size_t planetCount;
    std::vector<SolarData> set;
    SolarDataSet(size_t _setCount, size_t _planetCount)
    : setCount(_setCount), planetCount(_planetCount), set(_setCount) 
    {
        set.resize(_setCount);
        for(SolarData& p : set) {
            p.planets.resize(_planetCount);
        }
    }    
};

std::ostream& operator<<(std::ostream& os, const SolarDataSet& solarDataSets)
{
        os << "#include <cstdint>\n\n";
        os << "struct CBodyData {\n" 
           << "  // Horizon celestial body ID `pidx * 100 + 99`, i.e. pidx=1 for Mercury -> id=199\n"
           << "  unsigned id;\n"
           << "  // Position on the ecliptical plane w/ units in [km]\n"
           << "  double position[3];\n"
           << "  // Velocity vector on the ecliptical plane w/ units in [km/s]\n"
           << "  double velocity[3];\n"
           << "};\n";
        os << "struct SolarData {\n"
           << "  /// Timestamp in UTC, format YYYY-MM-DD HH:MM:SS\n"
           << "  const char* time_s;\n"
           << "  /// Seconds since Unix Epoch 1970-01-01T00:00:00.0Z in UTC\n"
           << "  int64_t time_u;\n"
           << "  CBodyData planets[" << solarDataSets.planetCount << "];\n"
           << "};\n";
        os << "struct SolarDataSet {\n"
           << "  /// Number of SolarData entries\n"
           << "  unsigned setCount;\n"
           << "  /// Number of CBodyData entries within each SolarData entry\n"
           << "  unsigned planetCount;\n"
           << "  SolarData set[" << solarDataSets.setCount << "];\n"
           << "};\n";
        os << "\n";
        os << "SolarDataSet solarDataSet = {\n"
           << "    /// Number of SolarData entries\n"
           << "    " << solarDataSets.setCount << ",\n"
           << "    /// Number of CBodyData entries within each SolarData entry\n"
           << "    " << solarDataSets.planetCount << ",\n"
           << "    /// SolarData entries\n"
           << "    {\n";
        
        constexpr int max_precision{std::numeric_limits<double>::digits10 + 1};
        os << std::setprecision( max_precision );
        for(size_t setIdx=0; setIdx < solarDataSets.setCount; ++setIdx) {
            const SolarData& ps = solarDataSets.set[setIdx];
            os << "        /** SolarData [" << setIdx << "]: " << ps.time_s << "( */" << "\n";
            os << "        { \"" << ps.time_s << "\", " << ps.time_u << ", {\n";
            for(size_t planetIdx=0; planetIdx < solarDataSets.planetCount; ++planetIdx) {
                const CBodyData& p = ps.planets[planetIdx];
                os << "            /** Planet [" << planetIdx << "], id " << p.id << " w/ max_precision " << max_precision << " */\n";
                os << "            { " << p.id << ",\n";
                os << "              { " << p.position[0] << ", " << p.position[1] << ", " << p.position[2] << "},\n";
                os << "              { " << p.velocity[0] << ", " << p.velocity[1] << ", " << p.velocity[2] << "}\n";
                if( planetIdx < solarDataSets.planetCount-1 ) {
                    os << "            },\n";
                } else {
                    os << "            }\n";
                }
            }
            if( setIdx < solarDataSets.setCount-1 ) {
                os << "        } },\n";
            } else {
                os << "        } }\n";
            }
        }   
        os << "    }\n";     
        os << "};\n";
        return os;  
}

int main(int argc, char** argv)
{
    if( argc == 2 ) {
        std::string filename = argv[1];
        std::string data = readFile(filename);
        std::cerr << "Parsing data file: " << filename << std::endl;
        double pos[3], velo[3];
        bool res = getPosVelo(data, pos, velo);
        return res ? 0 : 1;
    } 
    unsigned year_min = 2014;
    unsigned year_max = 2024;
    
    unsigned cbody_min = 1;
    unsigned cbody_max = 9;
    bool useBarycenter = false;
    
    if( argc >= 1+3 ) {
        // year_min, year_max, planet_count
        year_min = ::atoi(argv[1]);
        year_max = ::atoi(argv[2]);
        unsigned cbc = ::atoi(argv[3]);
        cbody_max = cbody_min + cbc - 1;
        if( year_max >= year_min && year_min > 0 && cbody_max >= cbody_min ) {
            std::cerr << "User args: "  
                      << "CBodies [" << cbody_min << ".." << cbody_max << "] for " 
                      << " years [" << year_min << ".." << year_max << "]" 
                      << std::endl;            
        } else {
            std::cerr << "Illegal user args: "  
                      << "CBodies [" << cbody_min << ".." << cbody_max << "] for " 
                      << " years [" << year_min << ".." << year_max << "]" 
                      << std::endl;            
            return 1;            
        }
    }
    if( argc > 1+3 ) {
        if( 0 == ::strcmp("-barycenter", argv[4]) ) {
            useBarycenter = true;
        }
    }
    const unsigned year_count = year_max - year_min + 1;
    const unsigned cbody_count = cbody_max - cbody_min + 1;
    
    SolarDataSet solarDataSets(year_count, cbody_count);
    
    int ret = 0;
    
    const unsigned request_count = year_count * cbody_count;
    std::cerr << "Requesting " << request_count << " data sets for " 
              << cbody_count << " cbodies [" << cbody_min << ".." << cbody_max << "] for " 
              << year_count << " years [" << year_min << ".." << year_max << "]"
              << ", barycenter " << std::to_string(useBarycenter) 
              << std::endl;
    
    std::atomic<unsigned> requestCompletedCount(0);
    std::atomic<unsigned> requestErrorCount(0);
    std::vector<jau::io::AsyncStreamResponseRef> responses;
    responses.reserve(10);
    
    std::vector<jau::io::net_tk_handle> free_handles;
    for(unsigned i=0; i<std::min<unsigned>(request_count, MaxConnections); ++i) {
        free_handles.push_back( jau::io::create_net_tk_handle() );
        // free_handles.push_back( nullptr );
    }
    auto get_free_handle = [&free_handles, &responses]() -> jau::io::net_tk_handle {
        if( 0 == free_handles.size() ) {
            jau::io::AsyncStreamResponseRef r = responses.back();
            responses.pop_back();
            r->thread.join();                
            return r->handle;
        } else {
            jau::io::net_tk_handle handle = free_handles.back();
            free_handles.pop_back();
            return handle;            
        }
    };
    
    for (unsigned year = year_min; year <= year_max; ++year) {
        SolarData& solarData = solarDataSets.set[year - year_min];
        std::string objectDate = toString(year, 1, 1);
        solarData.time_s.append(objectDate).append(" ").append(ZeroHour);
        solarData.time_u = toUnixSeconds(solarData.time_s);
         
        for (unsigned planetIdx = cbody_min; planetIdx <= cbody_max; ++planetIdx) {
            const size_t request_idx = ( year - year_min ) * cbody_count + ( planetIdx - cbody_min ) + 1;
            CBodyData& cbody = solarData.planets[planetIdx - cbody_min];
            if( useBarycenter ) {
                cbody.id = toBarycenterID(planetIdx);
            } else {
                cbody.id = toCBodyID(planetIdx);                
            }
            std::string objectID = toString(planetIdx);
            std::string cmd = getCommand(objectID, objectDate);
            std::cerr << "Request " << request_idx << " for CBody [" << planetIdx << "], id " << cbody.id << ", year " << year
                      << ", responses " << responses.size() << ", free handles " << free_handles.size() 
                      << std::endl;
            if constexpr (DBG_OUT) {
                std::cerr << cmd << std::endl << std::endl;
            }            
            
            jau::io::http::PostRequestPtr postReq = std::make_unique<jau::io::http::PostRequest>();
            postReq->header.emplace("Content-Type", "multipart/form-data; boundary="+HttpBoundary);
            postReq->body.append(HttpBoundarySep).append(HttpBoundary).append(CRLF);
            postReq->body.append("Content-Disposition: form-data; name=\"format\"").append(CRLF).append(CRLF);
            postReq->body.append("text").append(CRLF);
            postReq->body.append(HttpBoundarySep).append(HttpBoundary).append(CRLF);
            postReq->body.append(R"(Content-Disposition: form-data; name="input"; filename="a.cmd")").append(CRLF);
            postReq->body.append("Content-type: application/octet-stream").append(CRLF).append(CRLF);
            postReq->body.append(cmd).append(CRLF);
            postReq->body.append(HttpBoundarySep).append(HttpBoundary).append(HttpBoundarySep).append(CRLF);

            jau::io::AsyncStreamResponseRef response = jau::io::read_url_stream_async(get_free_handle(), HorizonUri, std::move(postReq), nullptr,
                                                        [year, &requestCompletedCount, &requestErrorCount, &cbody]
                                                        (jau::io::AsyncStreamResponse& response0, const uint8_t* data0 , size_t len0, bool is_final0) -> bool {
                const bool ok = response0.header_resp.completed() && 200 == response0.header_resp.response_code();                
                if( !ok ) {
                    std::cerr << "ERROR for CBody " << cbody.id << ", year " << year 
                             << ": status code " << response0.header_resp.response_code()
                             << ", result " << response0.result
                             << std::endl << std::endl;
                    ++requestErrorCount;
                    return false;
                } 
                if( nullptr != data0 && len0 > 0 ) {
                    response0.result_text.append(reinterpret_cast<const char*>(data0), len0);
                }
                if( is_final0 ) {
                    if constexpr (DBG_OUT) {
                        std::cerr << "Response for CBody " << cbody.id << ", year " << year 
                                 << ": status code " << response0.header_resp.response_code()
                                 << ", result " << response0.result
                                 << ", len " << len0 << "/" << response0.result_text.length()+len0 
                                 << ", read " << response0.total_read  << "/" << response0.content_length 
                                 << ", final " << is_final0
                                 << std::endl;
                    }
                    if( response0.result_text.length() > 0 ) {
                        double pos[3], velo[3];
                        bool res = getPosVelo(response0.result_text, pos, velo);
                        if( res ) {
                            cbody.position[0] = pos[0];
                            cbody.position[1] = pos[1];
                            cbody.position[2] = pos[2];
                            cbody.velocity[0] = velo[0];
                            cbody.velocity[1] = velo[1];
                            cbody.velocity[2] = velo[2];
                        } else {
                            std::cerr << "Parsing Data Error for CBody " << cbody.id << ", year " << year << std::endl; 
                        }
                    } else {
                        std::cerr << "No Data for CBody " << cbody.id << ", year " << year << std::endl << std::endl;
                    }
                    ++requestCompletedCount;
                }
                return true;
            });
            if( response ) { // g++ -Wnull-dereference
                if( responses.empty() ) {
                    responses.push_back(response);
                } else {
                    responses.insert(responses.begin(), response); // g++ -Wnull-dereference
                }
            }
        }
    }

    for(jau::io::AsyncStreamResponseRef& r : responses) {
        r->thread.join();
        jau::io::free_net_tk_handle( r->handle );         
    }
    responses.clear();
    for(jau::io::net_tk_handle h : free_handles) {
        jau::io::free_net_tk_handle(h);         
    }
    free_handles.clear();

    std::cerr << std::endl << "Requests completed " << requestCompletedCount << ", "; 
    if( 0 != requestErrorCount ) {
        std::cerr << "Errors: " << requestErrorCount << std::endl;
        
    } else {
        std::cerr << "OK" << std::endl;
        std::cout << solarDataSets;
    }
    
    return ret;    
}

