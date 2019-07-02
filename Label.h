
# ifndef LABEL_H
# define LABEL_H
# include <string>
# include <ostream>

class Label {
    unsigned _number;
    static unsigned _counter;

public:
    Label();
    unsigned number() const;
};

std::ostream &operator <<(std::ostream &ostr, const Label &label);

# endif