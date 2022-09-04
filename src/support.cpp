#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "support.h"

const char* month[12] = {
"Янв", "Фев", "Мар", "Апр", "Май", "Июн", "Июля", "Авг", "Сен", "Окт", "Ноя", "Дек"
};

const char* get_month(const tm* t)
{    
    int index = t->tm_mon;
    return month[index];
}

CurrentTime::CurrentTime() 
{
    time_t mytime = time(0);
    tm* td = localtime(&mytime);        
    sprintf(db, "%d %s %02d:%02d", td->tm_mday, get_month(td), td->tm_hour, td->tm_min);
}

CurrentTime::operator const char*()
{
    return db; 
}


Journal::Journal() : maxsize(30) 
{
}

void Journal::addToJournal(const char* label)
{
   int size = journal.size();
   if (size > maxsize)
   {
       size = size - maxsize;
       journal.erase(journal.begin(), journal.begin()+size);            
   }

   CurrentTime ct;
   std::string text(ct);
   text.append(": ");
   text.append(label);
   journal.push_back(text);
}

void Journal::sendToChar(char_data* ch, bool inverse_order)
{
    int count = journal.size();
    if (!count)
    {
        send_to_char("Журнал пуст.\r\n", ch);
        return;
    }

    for (int i=0; i<count; ++i)
    {
        int id = (inverse_order) ? (count-i-1) : i;
        std::string cmd(journal[id]);
        cmd.append("\r\n");
        send_to_char(cmd.c_str(), ch);
    }
}

void MemoryBuffer::alloc(int size)
{
    if (m_size >= size)
       { m_size = size; return; }
    char *new_data = new char[size];
    if (m_size != 0)        
       memcpy(new_data, m_pData, m_size);
    delete []m_pData;
    m_pData = new_data;
    m_size = size;
}

void TempBuffer::append(const char* data)
{
    int len = strlen(data);
    if (!buffer)
    {
        buffer_size = MAX(len, MAX_STRING_LENGTH);
        buffer = new char[buffer_size + 1];
        strcpy(buffer, data);
        string_size = len;
        return;
    }
        
    int free_size = buffer_size - string_size;
    if (free_size < len)
    {
        buffer_size = string_size + MAX_STRING_LENGTH;
        char *new_buffer = new char[buffer_size + 1];
        strcpy(new_buffer, buffer);
        delete []buffer;
        buffer = new_buffer;
    }

    char *p = buffer + string_size;
    strcpy(p, data);
    string_size += len;
}    
