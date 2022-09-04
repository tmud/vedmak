#ifndef _SUPPORT_H_
#define _SUPPORT_H_

const char* get_month(const tm* t);

class CurrentTime
{
public:
    CurrentTime();
    operator const char*();
private:
    char db[24];
};

class Journal
{
public:
    Journal();
    void addToJournal(const char* label);
    void sendToChar(char_data* ch, bool inverse_order);

private:
    std::vector<std::string> journal;
    int maxsize;
};


// Class for temporary memory buffers with autoclean
class MemoryBuffer
{
public:
    MemoryBuffer() : m_pData(NULL), m_size(0) {}
    MemoryBuffer(int size) : m_pData(NULL), m_size(0) { alloc(size); }
    ~MemoryBuffer() { delete []m_pData; }
    char* getData() const { return m_pData; }
    int getSize() const { return m_size; }
    void alloc(int size);

private:
    char* m_pData;
    int m_size;    
};


class TempBuffer
{
public:
    TempBuffer() : buffer(NULL), buffer_size(0), string_size(0) {}
    ~TempBuffer() { if (buffer) delete []buffer; }
    operator char*() const { return buffer; }
    int getSize() const { return string_size; }
    void append(const char* data);   
private:
    char* buffer;
    int buffer_size;
    int string_size;
};

#endif // _SUPPORT_H_
