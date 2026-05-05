#ifndef __PROGTEST__
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <iostream>
#endif /* __PROGTEST__ */

template <typename T>
static void my_swap(T& a, T&b) {
  T temp = a;
  a = b;
  b = temp;
}

class CFile
{
  public:
    CFile();
    bool           seek                       ( size_t           offset );
    size_t         read                       ( uint8_t           dst[],
                                                size_t            bytes );
    size_t         write                      ( const uint8_t     src[],
                                                size_t            bytes );
    void           truncate                   ();
    size_t         fileSize                   () const;
    void           addVersion                 ();
    bool           undoVersion                ();
  private:
    static const size_t CHUNK_SIZE = 4096;

    template<class T> struct SharedRef {
      T* pointer;
      SharedRef(T* p = nullptr) : pointer(p) {}
      SharedRef(const SharedRef& o) : pointer(o.pointer) {
        if (pointer) pointer->referenceCount++; 
      }
      ~SharedRef() {
        T* tmp = pointer;
        pointer = nullptr;
        if (tmp) {
          int ref = --tmp->referenceCount;
          if (ref == 0) delete tmp;
        }
      }
      SharedRef& operator = (SharedRef o) {
        my_swap(pointer, o.pointer);
        return *this;
      }
      T* operator -> () const { return pointer; }
      operator bool() const { return pointer != nullptr; }
    };

    struct BlockMath {
      size_t blockIndex;
      size_t offsetByte;
      size_t processByte;
    
      BlockMath (size_t currentPosition, size_t remainByte) {
        blockIndex = currentPosition / CHUNK_SIZE;
        offsetByte = currentPosition % CHUNK_SIZE;
        size_t remainSpace = CHUNK_SIZE - offsetByte;
        processByte = (remainByte < remainSpace) ? remainByte : remainSpace;
      }
    };

    struct DataBlock {
      uint8_t buffer[CHUNK_SIZE];
      int referenceCount = 1;

      DataBlock * clone() const {
        DataBlock * copy = new DataBlock();
        std::memcpy(copy->buffer, buffer, CHUNK_SIZE);
        return copy;
      }
    };

    struct FileState
    {
      SharedRef<DataBlock> * blockArray = nullptr;
      size_t capacity = 0;
      size_t content = 0;
      int referenceCount = 1;

      ~FileState() {delete[] blockArray;}
      
      FileState* clone() const {
        FileState * copy = new FileState();
        copy->capacity = capacity;
        copy->content = content;
        if (capacity > 0) {
          copy->blockArray = new SharedRef<DataBlock>[capacity];
          for (size_t i = 0; i < capacity; ++i) copy->blockArray[i] = blockArray[i];
        }
        return copy;
      }

      void ensureCapacity (size_t requiredBlock) {
        if (requiredBlock <= capacity) return;
        size_t newCapacity = capacity > 0 ? capacity * 2 : 8;
        while (newCapacity < requiredBlock) newCapacity *= 2;
        SharedRef<DataBlock> *newArray = new SharedRef<DataBlock>[newCapacity];
        for (size_t i = 0; i < capacity; ++i) newArray[i] = blockArray[i];
        delete[] blockArray;
        blockArray = newArray;
        capacity = newCapacity;
      }

      DataBlock* getMutableBlock (size_t idx) {
        SharedRef<DataBlock> & block = blockArray[idx];
        if(!block) block = new DataBlock();
        else if (block->referenceCount > 1) block = block->clone();
        return block.pointer;
      }

      void write(size_t& filePosition, const uint8_t source[], size_t totalBytes) {
        size_t blockNeeded = (filePosition + totalBytes + CHUNK_SIZE - 1) / CHUNK_SIZE; 
        ensureCapacity(blockNeeded);

        for (size_t bytesWritten = 0, currentStep = 0; 
                    bytesWritten < totalBytes; 
                    bytesWritten += currentStep, filePosition += currentStep) 
          {
            BlockMath _block(filePosition, totalBytes - bytesWritten);
            currentStep = _block.processByte;
            std::memcpy(getMutableBlock(_block.blockIndex)->buffer + _block.offsetByte, source + bytesWritten, currentStep);
          }

        if (filePosition > content) content = filePosition;
      }

      size_t read(size_t& filePosition, uint8_t destination[], size_t requestedBytes) {
        if (filePosition >= content) return 0;
        size_t actualBytesToRead = (filePosition + requestedBytes > content) ? content - filePosition : requestedBytes;
        for (size_t bytesRead = 0, currentStep = 0; 
                    bytesRead < actualBytesToRead; 
                    bytesRead += currentStep, filePosition += currentStep) 
        {
            BlockMath _block(filePosition, actualBytesToRead - bytesRead);
            currentStep = _block.processByte;
            if (blockArray[_block.blockIndex]) { 
                std::memcpy(destination + bytesRead, blockArray[_block.blockIndex]->buffer + _block.offsetByte, currentStep);
            }
        }
        return actualBytesToRead;
      }
    };

    struct VersionNode {
        SharedRef<FileState> savedState;
        size_t savedPosition;
        SharedRef<VersionNode> previousVersion;
        int referenceCount = 1;

        VersionNode(SharedRef<FileState> state, size_t pos, SharedRef<VersionNode> prev) 
            : savedState(state), savedPosition(pos), previousVersion(prev) {}

       ~VersionNode() {
            SharedRef<VersionNode> curr;
            my_swap(curr.pointer, previousVersion.pointer);
            
            while (curr.pointer && curr.pointer->referenceCount == 1) {
                SharedRef<VersionNode> next;
                my_swap(next.pointer, curr.pointer->previousVersion.pointer);
                curr = next;
            }
        }
    };

    SharedRef<FileState>   _currentState;
    size_t                 _filePointer;
    SharedRef<VersionNode> _historyHead;

    void prepareForModification();
  };

  CFile::CFile() : _currentState(new FileState()), _filePointer(0) {}
  
  void CFile::prepareForModification() {
    if(_currentState->referenceCount > 1) {
      _currentState = _currentState -> clone();
    }
  }

  bool CFile::seek(size_t offset) {
    if(offset > _currentState -> content) return false;
    _filePointer = offset;
    return true;
  }

  size_t CFile::read(uint8_t dest[], size_t bytes) {
    return _currentState->read(_filePointer,dest,bytes);
  }

  size_t CFile::write(const uint8_t src[], size_t bytes) {
    if(bytes == 0) return 0;
    prepareForModification();
    _currentState->write(_filePointer, src, bytes);
    return bytes;
  }

  void CFile::truncate() {
    prepareForModification();
    size_t startBlock = (_filePointer + CHUNK_SIZE - 1) / CHUNK_SIZE;
    for (size_t i = startBlock; i < _currentState->capacity; ++i) {
        _currentState->blockArray[i] = nullptr; 
    }
    _currentState->content = _filePointer;
  }

  size_t CFile::fileSize() const {
    return _currentState->content;
  }

  void CFile::addVersion() {
    _historyHead = new VersionNode(_currentState, _filePointer, _historyHead);
  }

  bool CFile::undoVersion() {
    if(!_historyHead) return false;
    _currentState = _historyHead->savedState;
    _filePointer  = _historyHead->savedPosition;
    _historyHead  = _historyHead->previousVersion;
    return true;
  }

#ifndef __PROGTEST__
bool               writeTest                  ( CFile           & x,
                                                const std::initializer_list<uint8_t> & data,
                                                size_t            wrLen )
{
  return x . write ( data . begin (), data . size () ) == wrLen;
}

bool               readTest                   ( CFile           & x,
                                                const std::initializer_list<uint8_t> & data,
                                                size_t            rdLen )
{
  uint8_t  tmp[100];
  uint32_t idx = 0;

  if ( x . read ( tmp, rdLen ) != data . size ())
    return false;
  for ( auto v : data )
    if ( tmp[idx++] != v )
      return false;
  return true;
}

int main ()
{
  CFile f0;

  assert ( writeTest ( f0, {10, 20, 30}, 3 ) );
  assert ( f0 . fileSize () == 3 );
  assert ( writeTest ( f0, {60, 70, 80}, 3 ) );
  assert ( f0 . fileSize () == 6 );
  assert ( f0 . seek ( 2 ));
  assert ( writeTest ( f0, {5, 4}, 2 ) );
  assert ( f0 . fileSize () == 6 );
  assert ( f0 . seek ( 1 ));
  assert ( readTest ( f0, {20, 5, 4, 70, 80}, 7 ));
  assert ( f0 . seek ( 3 ));
  f0 . addVersion();
  assert ( f0 . seek ( 6 ));
  assert ( writeTest ( f0, {100, 101, 102, 103}, 4 ) );
  f0 . addVersion();
  assert ( f0 . seek ( 5 ));
  CFile f1 ( f0 );
  f0 . truncate ();
  assert ( f0 . seek ( 0 ));
  assert ( readTest ( f0, {10, 20, 5, 4, 70}, 20 ));
  assert ( f0 . undoVersion () );
  assert ( f0 . seek ( 0 ));
  assert ( readTest ( f0, {10, 20, 5, 4, 70, 80, 100, 101, 102, 103}, 20 ));
  assert ( f0 . undoVersion () );
  assert ( f0 . seek ( 0 ));
  assert ( readTest ( f0, {10, 20, 5, 4, 70, 80}, 20 ));
  assert ( !f0 . seek ( 100 ));
  assert ( writeTest ( f1, {200, 210, 220}, 3 ) );
  assert ( f1 . seek ( 0 ));
  assert ( readTest ( f1, {10, 20, 5, 4, 70, 200, 210, 220, 102, 103}, 20 ));
  assert ( f1 . undoVersion () );
  assert ( f1 . undoVersion () );
  assert ( readTest ( f1, {4, 70, 80}, 20 ));
  assert ( !f1 . undoVersion () );
  return EXIT_SUCCESS;
}
#endif /* __PROGTEST__ */