#include <string>
#include <chrono>

#ifndef INFOTHREAD_H_
#define INFOTHREAD_H_

class InfoThread {
    private:

        /**
         * @brief PID or thread ID
         * 
         */
        long unsigned int m_pid;

        /**
         * @brief Thread status : PENDING | RUNNING | AVAIBLE
         * 
         */
        std::string m_status;

        /**
         * @brief Number of times the thread changes status
         * 
         */
        int m_count;

        /**
         * @brief Time to the last execution
         * 
         */
        std::time_t m_time;

        /**
         * @brief Duration to the last execution
         * 
         */
        long double m_duration;

    public:
        /**
         * @brief Construct a new Stat object
         * 
         */
        InfoThread(){};
        InfoThread(long unsigned int&);
        InfoThread(long unsigned int&, std::string);

        /**
         * @brief Destroy the Stat object
         * 
         */
        ~InfoThread(){};

        /**
         * @brief 
         * 
         * @return std::string
         */
        long unsigned int getPID();

        /**
         * @brief Get the Status object
         * 
         * @return std::string 
         */
        std::string getStatus();

        /**
         * @brief Set the Status object
         * 
         */
        void setStatus(std::string);

        /**
         * @brief Get the Count object
         * 
         * @return int 
         */
        int getCount();

        /**
         * @brief Set the Count object
         * 
         */
        void setCount();

        /**
         * @brief Set the Time object
         * 
         */
        void setTime(std::time_t);

        /**
         * @brief Get the Time object
         * 
         * @return long int
         */
        std::time_t getTime() {
            return m_time;
        }

        /**
         * @brief Get the Duration object
         * 
         * @return long double 
         */
        long double getDuration() {
            return m_duration;
        }

        /**
         * @brief Set the Duration object
         * 
         * @param double 
         */
        void setDuration(long double);

};

#endif /* INFOTHREAD_H_ */