#include <percy/percy.hpp>
#include <alice/alice.hpp>
#include <math.h> 
#include <algorithm> 
#include <vector>
#include <string>
#include <map>

#include <fmt/format.h>

#include "io.hpp"

std::vector<kitty::dynamic_truth_table> functions;

std::vector<std::string> 
split(const std::string& input, const std::string& regex) {
    // passing -1 as the submatch index parameter performs splitting
    std::regex re(regex);
    std::sregex_token_iterator first{ input.begin(), input.end(), re, -1 }, last;
    return { first, last };
}

using namespace percy;

namespace alice
{
    using kitty::dynamic_truth_table;

    ALICE_ADD_STORE(spec, 
            "spec", "s", "specification", "specification");
    
    ALICE_ADD_STORE(chain,
            "ntk", "n", "network", "networks");

    ALICE_DESCRIBE_STORE(chain, ntk)
    {
        return fmt::format("({}, {}, {})", 
                            ntk.get_nr_inputs(), 
                            ntk.get_nr_steps(), 
                            ntk.get_nr_outputs());
    }

    ALICE_PRINT_STORE(chain, os, ntk)
    {
        const auto nr_in = ntk.get_nr_inputs();
        char node_name = 'A' + nr_in;
        for (int i = 0; i < ntk.get_nr_steps(); i++) {
            os << node_name << " = " << kitty::to_binary(ntk.get_operator(i));
            auto fanins = ntk.get_step(i);
            for (auto fanin : fanins) {
                char fanin_name = '\0';
                if (fanin < nr_in) {
                    fanin_name = 'a' + fanin;
                } else {
                    fanin_name = 'A' + nr_in + (fanin - nr_in);
                }
                os << " " << fanin_name;
            }
            os << "\n";
            node_name++;
        }
    }

    /***************************************************************************
        Loads a truth table and adds it to the corresponding store.
    ***************************************************************************/
    class load_spec_command : public command
    {
        public:

            load_spec_command(const environment::ptr& env) : 
                command( env, "Create new specification" )
            {
                add_option(
                    "truth_table,--tt", truth_table, 
                    "truth table in hex format");
                add_flag( "--binary,-b", "read truth table as binary string" );
            }

        protected:

            void 
            execute() override
            {
                auto num_vars = 0u;
                dynamic_truth_table function;

                if (is_set( "binary" )) {     
                    num_vars = ::log(truth_table.size()) / ::log(2.0);
                    kitty::dynamic_truth_table f(num_vars);
                    kitty::create_from_binary_string(f, truth_table);
                    function = f;
                } else {
                    num_vars = ::log(truth_table.size() * 4) / ::log(2.0);
                    kitty::dynamic_truth_table f(num_vars);
                    kitty::create_from_hex_string(f, truth_table);
                    function = f;
                }
                functions.push_back(function);

                spec new_spec;
                new_spec[0] = function;

                this->store<spec>().extend() = new_spec;
            }

        private:
            std::string truth_table;
    };

    ALICE_DESCRIBE_STORE(spec, spec)
    {
        return fmt::format("({}, {}, {})", 
                            spec.get_nr_in(), 
                            spec.get_nr_out(), 
                            kitty::to_hex(spec[0]));
    }

    ALICE_PRINT_STORE(spec, os, spec)
    {
        os << "SPECIFICATION\n";
        os << "Nr. inputs = " << spec.get_nr_in() << "\n";
        os << "Nr. outputs = " << spec.get_nr_out() << "\n";
        for (int i = 0; i < spec.get_nr_out(); i++) {
            os << "f_" << i + 1 << " = ";
            os << kitty::to_hex(spec[i]) << " (hex) -- ";
            os << kitty::to_binary(spec[i]) << " (bin)\n";
        }
    }

    ALICE_ADD_COMMAND(load_spec, "Specification");

    class synthesize_command : public command
    {
        public:
            synthesize_command(const environment::ptr& env) : 
                command( env, "Synthesize network from specification" ),
                fanin_size(0)
            {
                add_option(
                    "fanin,-k", fanin_size, 
                    "fanin size of network operators");
            }

            void 
            execute() override
            {
                if (this->store<spec>().size()==0) {
                    this->env->err() << "Error: specification not found\n";
                    return;
                }
                if (fanin_size <= 1 || fanin_size > 5) {
                    this->env->err() << "Error: fanin size " << fanin_size
                        << " is not supported\n";
                    return;
                }
                auto synth_spec = this->store<spec>().current();
                chain c;

                auto result = synthesize(synth_spec, c);
                this->store<chain>().extend() = c;

                switch (result) {
                    case synth_result::success:
                        this->env->out() << "SUCCESS\n";
                        break;
                    case synth_result::failure:
                        this->env->out() << "FAILURE\n";
                        break;
                    case synth_result::timeout:
                        this->env->out() << "TIMEOUT\n";
                        break;
                }
                //this->env->out() << "Time elapsed: " << spec.synth_time 
                   // << "\u03BCs\n";
            }
        private:
            int fanin_size;
    };
	
   
    class fiwls2018_command : public command
    {
        public:
            fiwls2018_command(const environment::ptr& env) : 
                command( env, "Synthesize network from specification for IWLS 2018 contest" )
        {
            add_option( "filename, -f", filename, "Benchmarks.txt file"); 
        }

        void 
        execute() override
        {
            std::ifstream file(filename);

            spec synth_spec;
            chain c;

            bsat_wrapper solver;
            knuth_encoder encoder(solver);

            std::string line;
            std::vector<std::string> sort_inputs;
            while (std::getline(file, line))
            {	
                sort_inputs.push_back(line); 
            }
            using string_t = decltype(line); 
            std::sort(
                    sort_inputs.begin(), 
                    sort_inputs.end(), 
                    [](const string_t& p1, const string_t& p2 ) { 
                    return p1.size() < p2.size(); 
                    } 
                    );

            for (auto& line : sort_inputs)		
            {
                const auto inputs = split(line, "\s");
                if ((inputs[0] == "#") || (inputs.size() < 3)) {
                    continue;
                }

                assert (inputs.size() == 3); 
                auto truth_table = inputs[0]; 
                std::string outfile_name; 

                outfile_name += truth_table;

                auto const fanin_size = std::stoi (inputs[1]); 
                outfile_name+=fmt::format( "-{}",inputs[1]); 
                auto const gates_size = std::stoi (inputs[2]); 
                outfile_name+=fmt::format( "-{}.bln",inputs[2]);  
                auto num_vars = log2(truth_table.size() << 2); 

                std::ofstream outfile (outfile_name); 

                kitty::dynamic_truth_table tt(num_vars);
                kitty::create_from_hex_string(tt, truth_table);

                synth_spec[0] = tt;
                synth_spec.initial_steps = gates_size;
                synth_spec.fanin = fanin_size;
                synth_spec.add_colex_clauses = false;
                synth_spec.add_lex_clauses = true;

                encoder.reset();
                while (next_solution(synth_spec, c, solver, encoder, SYNTH_STD_CEGAR) == success) {
                    if (c.satisfies_spec(synth_spec)) {
                        to_iwls(c, outfile);
                        outfile << std::endl;
                    }
                }
            }
        }
        private:
            std::string filename;
    };

    class iwls2018_command : public command
    {
    public:
        iwls2018_command(const environment::ptr& env) :
            command(env, "Synthesize network from specification for IWLS 2018 contest")
        {
            add_option( "truth-table, -t", truth_table, "Function truth table"); 
            add_option( "fanin, -f", fanin_str, "Number of operator fanins"); 
            add_option( "gates, -g", gates_str, "Number of gates"); 
        }

        void 
        execute() override
        {
            if (truth_table.size() == 0 || fanin_str.size() == 0 || gates_str.size() == 0) {
                fprintf(stderr, "Usage: iwls2018 -t [truth table] -f [fanin size] -g [nr. of gates]\n");
                return;
            }
            std::string outfile_name(truth_table);

            auto const fanin_size = std::stoi(fanin_str); 
            outfile_name+=fmt::format( "-{}",fanin_size); 
            auto const gates_size = std::stoi(gates_str); 
            outfile_name+=fmt::format( "-{}.bln",gates_size);  
            auto num_vars = log2(truth_table.size() << 2); 

            std::ofstream outfile (outfile_name); 

            kitty::dynamic_truth_table tt(num_vars);
            kitty::create_from_hex_string(tt, truth_table);

            int nr_solutions = 0;

            spec synth_spec; // Create specification
            synth_spec[0] = tt;
            synth_spec.add_colex_clauses = false;
            synth_spec.add_lex_clauses = true;
            synth_spec.fanin = fanin_size;
            synth_spec.initial_steps = gates_size;

            chain c; // Holds the synthesized network
            cmsat_wrapper solver; // Use the CryptoMinisat solver
            knuth_encoder encoder(solver); // Use Knuth's CNF encoding

            while (next_solution(synth_spec, c, solver, encoder) == success) {
                printf("%d\n", nr_solutions++); // Give feedback
                if (c.satisfies_spec(synth_spec)) { // Only write correct solutions
                    to_iwls(c, outfile);
                    outfile << std::endl;
                }
            }
        }

        private:
            std::string truth_table;
            std::string fanin_str;
            std::string gates_str;
    };
    
    ALICE_ADD_COMMAND(fiwls2018, "Read IWLS 2018 contest file");

    ALICE_ADD_COMMAND(iwls2018, "Synthesize IWLS 2018 contest spec");
	
    ALICE_ADD_COMMAND(synthesize, "Synthesis");

    class cnf_gen_command : public command
    {
    public:
        cnf_gen_command(const environment::ptr& env) :
            command(env, "Synthesize network from specification for IWLS 2018 contest")
        {
            add_option("truth-table, -t", truth_table, "Function truth table");
            add_option("fanin, -f", fanin_str, "Number of operator fanins");
            add_option("gates, -g", gates_str, "Number of gates");
        }

        void
        execute() override
        {
            spec spec;

            if (truth_table.size() == 0) {
                fprintf(stderr, "Error: truth table not specified\n");
                return;
            }
            if (gates_str.size() == 0) {
                fprintf(stderr, "Error: number of gates not specified\n");
                return;
            }
            auto nr_gates = std::stoi(gates_str);
            if (truth_table.substr(0, 3) == "maj") {
                // Encode synthesis for a majority graph
                const auto nr_inputs = std::stoi(truth_table.substr(3, 1));
                kitty::dynamic_truth_table maj_tt(nr_inputs);
                kitty::create_majority(maj_tt);
                spec.fanin = 3;
                spec.nr_steps = nr_gates;
                spec[0] = maj_tt;
                spec.preprocess();
                cnf_formula formula;
                maj_encoder encoder(formula);
                encoder.encode(spec);

                const auto dimacs_filename = "maj-" + std::to_string(nr_inputs) + 
                    "-" + std::to_string(nr_gates) + ".cnf";
                auto fhandle = fopen(dimacs_filename.c_str(), "w");
                if (fhandle == NULL) {
                    fprintf(stderr, "Error: unable to open output file\n");
                    return;
                }
                formula.to_dimacs(fhandle);
                fclose(fhandle);
            }
        }

    private:
            std::string truth_table;
            std::string fanin_str;
            std::string gates_str;
    };

    ALICE_ADD_COMMAND(cnf_gen, "Generate DIMACS file from specification");
    
    ALICE_ADD_STORE(std::vector<partial_dag>,
            "pds", "p", "partial_dags", "partial_dags");

    ALICE_DESCRIBE_STORE(std::vector<partial_dag>, dags)
    {
        return fmt::format("[{}]", dags.size());
    }

    /// A command to generate partial DAGs of the specified size.
    /// Generated PDs are loaded into the appropriate store after generation.
#ifndef DISABLE_NAUTY
    class pd_gen_command : public command
    {
    public:
        pd_gen_command(const environment::ptr& env) :
            command(env, "Generate partial DAGs")
        {
            add_option("gates, -g", gates_str, "Generate PDs with this number of gates");
            add_option("max-gates, -m", max_gates_str, "Generate PDs with up to this number of gates");
        }

        void execute() override
        {
            const auto min_nr_gates = 1;
            auto nr_gates = std::atoi(gates_str.c_str());
            auto max_nr_gates = std::atoi(max_gates_str.c_str());
            if (max_nr_gates > 0) {
                for (int i = min_nr_gates; i <= max_nr_gates; i++) {
                    printf("generating PDs of size %d\n", i);
                    const auto filename = "pd" + std::to_string(i) + ".bin";
                    pd_write_nonisomorphic(i, filename.c_str());
                }
            } else if (nr_gates > 0) {
                printf("generating PDs of size %d...\n", nr_gates);
                const auto filename = "pd" + std::to_string(nr_gates) + ".bin";
                pd_write_nonisomorphic(nr_gates, filename.c_str());
            } else {
                fprintf(stderr, "Error: incorrect number of gates\n");
            }
        }

    private:
        std::string gates_str;
        std::string max_gates_str;

    };
    
    ALICE_ADD_COMMAND(pd_gen, "Generate partial DAGs");
#endif // DISABLE_NAUTY

    class pd_load_command : public command
    {
    public:
        pd_load_command(const environment::ptr& env) : 
            command(env, "Load a set of partial DAGs")
        {
            add_option("gates, -g", gates_str, "Load PDs with this number of gates");
        }

        void execute() override
        {
            auto nr_gates = std::atoi(gates_str.c_str());
            if (nr_gates > 0) {
                const auto filename = "pd" + std::to_string(nr_gates) + ".bin";
                const auto dags = read_partial_dags(filename.c_str());
                printf("Read %zu dags\n", dags.size());
                this->store<std::vector<partial_dag>>().extend() = dags;
            } else {
                fprintf(stderr, "Error: incorrect number of gates\n");
            }
        }

    private:
        std::string gates_str;
    };
    
    ALICE_ADD_COMMAND(pd_load, "Load partial DAGs");

    class pd_count_command : public command
    {
    public:
        pd_count_command(const environment::ptr& env) : 
            command(env, "Counts the number of partial DAGs in a file")
        {
            add_option("filename, -g", filename, "File containing PDs");
        }

        void execute() override
        {
            auto fhandle = fopen(filename.c_str(), "rb");
            if (fhandle == NULL) {
                fprintf(stderr, "Error: unable to open file\n");
                return;
            }
            const auto nr_dags = count_partial_dags(fhandle);
            fclose(fhandle);
            printf("File contains %zu dags\n", nr_dags);
        }

    private:
        std::string filename;
    };

    ALICE_ADD_COMMAND(pd_count, "Count partial DAGs in file");
}

ALICE_MAIN(percy)

