#include <Rcpp.h>
#include <zlib.h>
#include <iostream>
#include <fstream>


// [[Rcpp::export]]
int filterCells(
    std::string fragments,
    std::string outfile,
    std::vector<std::string> keep_cells,
    bool verbose = true
) {
  // opening gzipped compressed stream
  gzFile ifileHandler = gzopen(fragments.c_str(), "rb");
  gzFile ofileHandler = gzopen(outfile.c_str(), "wb");

  // return 1 if it can't find the file
  if (ifileHandler == NULL || ofileHandler == NULL) {
    Rcpp::Rcerr << "can't open file" << std::flush;
    return 1;
  }

  // C based buffered string parsing
  char* cb_char;
  size_t line_counter {1};
  // @Tim: This Assumes the max length of a file can
  // be 256 characters. If not then the parser
  // can artificially break a line.
  uint32_t buffer_length = 256;
  char *buffer = new char[buffer_length];

  // Hash Map storing the barcodes to keep
  std::unordered_map<std::string, size_t> index_hash;

  size_t num_whitelist_cells {0};
  {
    for (size_t i=0; i<keep_cells.size(); i++) {
      index_hash[keep_cells[i]] = i;
    }
    
    if (verbose) {
      num_whitelist_cells = index_hash.size();
      Rcpp::Rcerr << "Keeping " << num_whitelist_cells
                  << " cell barcodes"
                  << std::endl << std::flush;
    }
  }

  // char * to string extraction
  std::string cb_seq, line_seq;
  cb_seq.reserve(32);
  line_seq.reserve(buffer_length);

  // looping over the fragments file
  while(gzgets(ifileHandler, buffer, buffer_length) !=0 ){
    line_seq.clear();
    line_seq.append(buffer);
    
    cb_char = strtok ( buffer, "\t" );
    
    for (auto i=1; i<=3; i++) {
      cb_char = strtok (NULL, "\t");
      
      if(i == 3) {
        cb_seq.clear();
        cb_seq.append(cb_char);
        if (index_hash.find(cb_seq) != index_hash.end()) {
          bool write_ok = gzputs(ofileHandler, line_seq.c_str());
          if (write_ok <= 0) {
            Rcpp::Rcerr << "Can't write into the output file"
                        << std::endl << std::flush;
            return 1;
          }
        }
      }
    }
    
    line_counter += 1;
    bool is_ten_mil = line_counter % 10000000 == 0; 
    if (verbose) {
      if (is_ten_mil) {
        Rcpp::Rcerr << "\r                                                  ";
      }

      if (line_counter % 1000000 == 0) {
        Rcpp::Rcerr << "\rDone Processing " << line_counter / 1000000
                    << " million lines";
      }
    }
    
    if (is_ten_mil) {
      Rcpp::checkUserInterrupt();
    }
  }

  //Cleanup
  gzclose(ifileHandler);
  gzclose(ofileHandler);

  return 0;
}
