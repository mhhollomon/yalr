#if !defined(YALR_AST_HPP)
#define YALR_AST_HPP

#include <boost/fusion/include/io.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

#include <iostream>
#include <string>

namespace yalr { namespace ast {

    struct employee
    {
        int age;
        std::string forename;
        std::string surname;
        double salary;
    };

    using boost::fusion::operator<<;

}}

BOOST_FUSION_ADAPT_STRUCT(yalr::ast::employee,
    age, forename, surname, salary)


#endif
