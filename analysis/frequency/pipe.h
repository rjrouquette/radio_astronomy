//
// Created by robert on 6/4/20.
//

#ifndef FREQUENCY_RESPONSE_PIPE_H
#define FREQUENCY_RESPONSE_PIPE_H


class pipe {
private:
    double value;

public:
    pipe();
    virtual ~pipe() = default;

    double getValue() const;
    void setValue(double value);
};


#endif //FREQUENCY_RESPONSE_PIPE_H
