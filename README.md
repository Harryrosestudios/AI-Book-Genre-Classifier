# Book Collection Scanner with AI Genre Classification

A command-line application that scans ISBN barcodes and automatically organises your book collection by genre using AI-powered classification.

## What It Does

This application combines barcode scanning, book metadata retrieval, and AI-powered genre classification to help you digitally catalogue and organise your physical book collection:

- **ISBN Scanning**: Works with barcode scanners that input text (like the [NETUM NT-2012](https://www.amazon.co.uk/NETUM-Handheld-Portable-Supermarket-NT-2012/dp/B01FADCQHG?source=ps-sl-shoppingads-lpcontext&ref_=fplfs&smid=A1XI5HHVMNOLNL&th=1)) or manual ISBN entry
- **Book Metadata Retrieval**: Fetches title and author information from OpenLibrary API
- **AI Genre Classification**: Uses Facebook's BART model via Hugging Face to intelligently classify books into genres
- **Smart Organisation**: Automatically sorts books by genre and author in a formatted text file
- **Dynamic Genre Management**: Learns new genres and adds them to your classification system
- **Enhanced Context**: Incorporates OpenLibrary subject data for more accurate AI classifications

## Why It's Useful

**Personal Book Organisation** (Primary Use Case):
- Perfect for organising your own collection - I'm currently installing a DIY "built-in" bookcase into my living room, which served as my motivation to creating this app
- Eliminates manual data entry - just scan and go
- Creates organised, searchable catalogues of large personal collections
- Provides consistent genre classification across your entire library
- Generates ready-to-use inventory files for insurance or cataloguing purposes

**For Book Collectors**:
- Rapid inventory management for extensive collections
- Consistent categorisation without manual effort
- Easy integration with existing barcode scanner hardware

**For Libraries and Bookstores**:
- Professional inventory management
- Consistent categorisation across staff members
- Streamlined cataloguing process

**For Digital Organisation**:
- Creates formatted text files perfect for sharing or importing into other systems
- Maintains genre consistency with AI-powered classification
- Automatically handles both ISBN-10 and ISBN-13 formats

## Development Challenges and Solutions

### Challenge 1: Model Selection and API Compatibility
**Issue**: Initially chose Google's FLAN-T5-base model, but encountered persistent 404 errors from Hugging Face Inference API despite the model existing.

**Solution**: Switched to Facebook's BART-large-mnli model, which is specifically designed for zero-shot classification tasks and has reliable API availability. This actually improved results as BART is purpose-built for classification rather than general text generation.

### Challenge 2: Genre Classification Accuracy
**Issue**: AI was making logical but suboptimal choices (e.g., classifying "Haunting Adeline" as Horror instead of Dark Romance based solely on the word "Haunting"). Had to get this right for the BookTok girlies who would never forgive such a classification sin.

**Solution**: Enhanced the input context by:
- Extracting relevant subjects from OpenLibrary metadata
- Implementing confidence threshold logic to prefer more specific genres when scores are close
- Adding genre preference rules (Dark Romance over Horror, Classic Literature over Literary Fiction)

### Challenge 3: API Authentication and Token Management
**Issue**: Accidentally used placeholder tokens and experienced authentication failures.

**Solution**: Implemented proper token validation and clear setup instructions. Added debugging capabilities during development to quickly identify authentication issues.

### Challenge 4: ISBN Format Handling
**Issue**: Barcode scanners can input ISBNs in various formats (with/without hyphens, ISBN-10 vs ISBN-13), and partial scans could trigger premature API calls.

**Solution**: Created robust ISBN normalisation that:
- Strips non-essential characters
- Validates length before processing
- Waits for complete input before making API calls
- Handles both ISBN-10 and ISBN-13 formats seamlessly

### Challenge 5: Response Format Variability
**Issue**: Different AI models return responses in different JSON structures, making parsing unreliable.

**Solution**: Implemented comprehensive response parsing that handles multiple format types and gracefully falls back to defaults when parsing fails.

## Installation and Setup

### Prerequisites
- C++ compiler with C++11 support
- libcurl development library
- jsoncpp development library
- Hugging Face account and API token

### macOS Installation
```bash
# Install dependencies using Homebrew
make install_deps

# Compile the application
make

# Run the scanner
./book_scanner
```

### Linux Installation
```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install build-essential libcurl4-openssl-dev libjsoncpp-dev

# CentOS/RHEL/Fedora
sudo yum install gcc-c++ libcurl-devel jsoncpp-devel
# or for newer versions:
sudo dnf install gcc-c++ libcurl-devel jsoncpp-devel

# Compile
g++ -std=c++11 -Wall -o book_scanner book_scanner.cpp -lcurl -ljsoncpp

# Run
./book_scanner
```

### Windows Installation
```bash
# Using MSYS2/MinGW-w64
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-curl mingw-w64-x86_64-jsoncpp

# Or using vcpkg
vcpkg install curl jsoncpp

# Compile (adjust paths as needed)
g++ -std=c++11 -Wall -o book_scanner.exe book_scanner.cpp -lcurl -ljsoncpp

# Run
./book_scanner.exe
```

### Configuration
1. Get your Hugging Face API token from [Settings → Access Tokens](https://huggingface.co/settings/tokens)
2. Replace `hf_YOUR_TOKEN_HERE` in the source code with your actual token
3. Recompile: `make clean && make` (or equivalent for your platform)

## Usage

1. Start the application: `./book_scanner`
2. Scan ISBN barcodes with your scanner or type them manually
3. The app will automatically fetch book data and classify genres
4. Press Ctrl+C to save and exit gracefully
5. Check `book_collection.txt` for your organised collection

## Future Updates

### Planned Features
- **Database Integration**: SQLite storage for more complex queries and reporting
- **Web Interface**: Browser-based GUI for easier management and visualisation
- **Export Options**: CSV, JSON, and spreadsheet format exports
- **Batch Processing**: Upload CSV files of ISBNs for bulk processing
- **Advanced Filtering**: Search and filter by author, publication date, or custom tags
- **Collection Analytics**: Statistics on genre distribution, author frequency, publication trends

### Technical Improvements
- **Caching System**: Local cache of previously processed books to reduce API calls
- **Multiple API Sources**: Fallback to Google Books API when OpenLibrary fails
- **Enhanced AI Context**: Use book descriptions and reviews for even better genre classification
- **Custom Genre Training**: Allow users to manually correct classifications to improve future results

### Hardware Integration
- **Multiple Scanner Support**: Support for different barcode scanner models and protocols
- **Mobile App**: Companion smartphone app for portable scanning
- **Label Generation**: Automatic spine label creation for physical organisation

### Data Features
- **Collection Sharing**: Export/import collections between users
- **Duplicate Detection**: Identify and handle duplicate entries
- **Series Detection**: Automatically group books in series
- **Reading Status**: Track read/unread status and personal ratings

The application is designed with modularity in mind, making these future enhancements straightforward to implement while maintaining the core functionality that users depend on.
