#pragma once

#include <percy/chain.hpp>

namespace percy
{
    /***************************************************************************
        Functions to convert a chain to the output format for the IWLS 2018
        programming competition.
    ***************************************************************************/
    template<int FI>
    void 
    to_iwls(const chain<FI>& c, std::ostream& s)
    {
        assert(c.get_nr_outputs() == 1);
        const auto nr_inputs = c.get_nr_inputs();

        c.foreach_vertex([&c, &s, nr_inputs] (auto step, int i) {
            s << std::endl;
            assert(nr_inputs + i >= nr_inputs) ; 
            s << char(('A' + nr_inputs + i)); 
            s << " = "; 

            kitty::print_binary(c.get_operator(i), s);
            s << " "; 
            switch (FI) {
                case 3: 
                    if (step[0] >= nr_inputs)
                        s << char(('A' + step[0]));
                    else 
                        s << char(('a' + step[0]));
					s << " ";
                    if (step[1] >= nr_inputs)
                        s << char(('A' + step[1]));
                    else 
                        s << char(('a' + step[1]));
					s << " ";
                    if (step[2] >= nr_inputs)
                        s << char(('A' + step[2]));
                    else 
                        s << char(('a' + step[2]));
                break; 
                case 4: 
                    if (step[0] >= nr_inputs)
                        s << char(('A' + step[0]));
                    else 
                        s << char(('a' + step[0]));
					s << " ";
                    if (step[1] >= nr_inputs)
                        s << char(('A' + step[1]));
                    else 
                        s << char(('a' + step[1]));
					s << " ";
                    if (step[2] >= nr_inputs)
                        s << char(('A' + step[2]));
                    else 
                        s << char(('a ' + step[2]));
					s << " ";
                    if (step[3] >= nr_inputs)
                        s << char(('A' + step[3]));
                    else 
                        s << char(('a' + step[3]));
                    break;  
                default:
                    std::cerr << "Unsupported fanin size " << FI << std::endl;
                    exit(1);
            }
        });
    }

    template<>
    void 
    to_iwls(const chain<2>& c, std::ostream& s)
    {
        assert(c.get_nr_outputs() == 1);
        const auto nr_inputs = c.get_nr_inputs();

        c.foreach_vertex([&c, &s, nr_inputs] (auto step, int i) {
            s << std::endl;
            s << char(('A' + nr_inputs + i)); 
            s << " = "; 

            kitty::print_binary(c.get_operator(i), s);
            s << " ";

            if (step.first >= nr_inputs) {
                s << char(('A' + step.first));
            } else {
                s << char(('a' + step.first));
            }
			s << " ";
            if (step.second >= nr_inputs) {
                s << char(('A' + step.second));
            } else  {
                s << char(('a' + step.second));
            }
        });
    }

}

