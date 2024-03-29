import gdb
pp = gdb.printing.RegexpCollectionPrettyPrinter("amu")

import time

def dbgmsg(s):
    t = time.perf_counter()
    gdb.write(f"dbg:{t}: {s}"); gdb.flush()

def full_deref_if_ptr(thing):
    while thing.type.code == gdb.TYPE_CODE_PTR:
        if thing.address == 0:
            return 'null'
        else:
            thing = thing.dereference()
    return thing

def is_tnode(val):
    n = str(val.type)
    return n == "amu::TNode" or n == "amu::TNode *"

class print_lnode_chain(gdb.Command):
    def __init__(self):
        super(print_lnode_chain, self).__init__("plnode", gdb.COMMAND_USER, gdb.COMPLETE_EXPRESSION)

    def invoke(self, arg, tty):
        try:
            val = gdb.parse_and_eval(arg)
            if val is None:
                print("invalid expression")
                return
            out = f"{val} -> "
            iter = val['next'].dereference()
            while iter.address != val:
                out += f"{iter.address} -> "
                iter = iter['next'].dereference()
            gdb.write(out)
        except Exception as e:
            print(f"{self.__class__.__name__} error: ", e)
print_lnode_chain()

class String_printer:
    def __init__(self, val):
        self.val = val
    
    def to_string(self):
        try:
            val:gdb.Value = self.val
            ptr = int(val['str'])
            if abs(val['count']) > int(10000):
                return "corrupt String"
            if val['count'] == 0:
                return "empty"
            buf = gdb.selected_inferior().read_memory(ptr, val['count']).tobytes().decode()
            buf = buf.replace('\n', '\\n')
            buf = f"\"{buf}\""
            return buf
        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
pp.add_printer("String", r"^amu::String$", String_printer)

class DString_printer:
    def __init__(self, val):
        self.val = val
    
    def to_string(self):
        try:
            val = self.val
            return gdb.parse_and_eval(f"((DString*){val.address})->get_string()")
        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
pp.add_printer("DString", r"^amu::DString$", DString_printer)

class Array_printer:
    def __init__(self, val): 
        self.val = val
    
    def display_hint(self):
        return 'array'

    def to_string(self):
        try:
            val = self.val
            if not val['count']:
                return "{empty}"
            type = str(val.type)
            subtype = type[type.find("<")+1:-1]
            ptr = val['data']
            out = "{\n"
            for i in range(val['count']):
                hi = gdb.parse_and_eval(f"*((({subtype}*){ptr})+{i})");
                if str(hi) == '0x0':
                    out += "  (nullptr),\n"
                else:
                    out += "  " + str(full_deref_if_ptr(hi)) + ",\n"
            out += "}"
            return out
            # return gdb.parse_and_eval(f"*(({subtype}*){val['data']})@{val['count']}")
        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
pp.add_printer("Array", r"^amu::Array<.*>$", Array_printer)

class MessagePart_printer:
    def __init__(self, val): 
        self.val = val

    def to_string(self):
        try:
            val:gdb.Value = self.val
            print(str(val['kind']))
            match str(val['kind']):
                case "amu::messagepart::plain": 
                    return f"plain: {val['plain']}"
                case "amu::messagepart::path": 
                    return f"path: {val['plain']}"
                case "amu::messagepart::token": 
                    return f"token: {val['token']}"
                case "amu::messagepart::source": return f"source: {val['source'].dereference()}"
                case "amu::messagepart::entity": return f"entity"
                case "amu::messagepart::label": return f"label: {val['label'].dereference()}"
                case "amu::messagepart::code": return f"code"
        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
pp.add_printer("MessagePart", r"^amu::MessagePart$", MessagePart_printer)

class Token_printer:
    def __init__(self, val):
        self.val = val
    
    def to_string(self):
        try:
            val:gdb.Value = self.val
            s = gdb.execute(f"call *to_string((amu::Token*){val.address})", to_string=True)
            return s[s.find('=')+2:-1]
        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
pp.add_printer("Token", r"^amu::Token$", Token_printer)

class Label_printer:
    def __init__(self, val): self.val = val

    def to_string(self):
        try:
            val:gdb.Value = self.val
            s = gdb.execute(f"call *((amu::Label*){val.address})->dump()", to_string=True)
            return s[s.find('=')+2:-1]
        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
pp.add_printer("Label", r"^amu::Label$", Label_printer)

class Statement_printer:
    def __init__(self, val): self.val = val

    def to_string(self):
        try:
            val:gdb.Value = self.val
            out = f"{str(val['kind'])[5:].replace('statement', 'stmt')} "
            start = ""
            end = ""
            if val['node']['start']:
                start = str(val['node']['start'].dereference())
            else:
                start = "<null start>"
            if val['node']['end']:
                end = str(val['node']['end'].dereference())
            else:
                end = "<null end>"
            out += f"{start} -> {end}"
            return out
        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
pp.add_printer("Statement", r"^amu::Statement$", Statement_printer)

class Function_printer:
    def __init__(self, val): self.val = val

    def to_string(self):
        try:
            val:gdb.Value = self.val
            out = "function "
            start = ""
            end = ""
            if val['node']['start']:
                start = str(val['node']['start'].dereference())
            else:
                start = "<null start>"
            if val['node']['end']:
                end = str(val['node']['end'].dereference())
            else:
                end = "<null end>"
            out += f"{start} -> {end}"
            return out
        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
pp.add_printer("Function", r"^amu::Function$", Function_printer)

class Module_printer:
    def __init__(self, val): self.val = val

    def to_string(self):
        try:
            val:gdb.Value = self.val
            out = "module "
            start = ""
            end = ""
            if val['node']['start']:
                start = str(val['node']['start'].dereference())
            else:
                start = "<null start>"
            if val['node']['end']:
                end = str(val['node']['end'].dereference())
            else:
                end = "<null end>"
            out += f"{start} -> {end}"
            return out
        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
pp.add_printer("Module", r"^amu::Module$", Module_printer)

class Pool_printer:
    def __init__(self,val): self.val = val
    def to_string(self):
        try:
            val:gdb.Value = self.val
            out = ""
            out += f"n per chunk: {val['items_per_chunk']}\n"
            out += f"chunks: {val['chunk_root']}\n"
            out += f"free blocks: {val['free_blocks']}\n"
            out += f"items: {val['items']}"
            return out
        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
pp.add_printer("Pool", r"^amu::Pool<.*>$", Pool_printer)

class Tuple_printer:
    def __init__(self,val): self.val = val
    def to_string(self):
        try:
            val:gdb.Value = self.val
            out = f"{str(val['kind'])[5:]} "
            start = ""
            end = ""
            if val['node']['start']:
                start = str(val['node']['start'].dereference())
            else:
                start = "<null start>"
            if val['node']['end']:
                end = str(val['node']['end'].dereference())
            else:
                end = "<null end>"
            out += f"{start} -> {end}"
            return out
        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
pp.add_printer("Tuple", r"^amu::Tuple$", Tuple_printer)

class ASTNode_printer:
    def __init__(self, val): self.val = val
    def to_string(self):
        try:
            val:gdb.Value = self.val
            s = gdb.execute(f"call *((amu::ASTNode*){val.address})->dump()", to_string = True)
            return s[s.find('=')+2:-1]
        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
pp.add_printer("TNode", r"^amu::ASTNode", ASTNode_printer)

class Expression_printer:
    def __init__(self, val): self.val = val
    def to_string(self):
        try:
            val:gdb.Value = self.val
            s = gdb.execute(f"call *((amu::Expr*){val.address})->dump()", to_string=True)
            return s[s.find('=')+2:-1]
        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
pp.add_printer("Expression", r"^amu::Expr$", Expression_printer)

class Map_printer: 
    def __init__(self, val): self.val = val
    def to_string(self):
        try:
            val:gdb.Value = self.val

        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
# pp.add_printer("Map", r"^amu::Map<.*?,.*>$", Map_printer)

class TAC_printer:
    def __init__(self, val): self.val = val
    def to_string(self):
        try:
            val:gdb.Value = self.val
            s = gdb.execute(f"call *to_string((amu::TAC*){val.address})", to_string = True)
            return s[s.find('=')+2:-1]
        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
pp.add_printer("TAC", r"^amu::TAC", TAC_printer)

class Arg_printer:
    def __init__(self, val): self.val = val
    def to_string(self):
        try:
            val:gdb.Value = self.val
            s = gdb.execute(f"call *to_string((amu::Arg*){val.address})", to_string = True)
            return s[s.find('=')+2:-1]
        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
pp.add_printer("Arg", r"^amu::Arg$", Arg_printer)

class BC_printer: 
    def __init__(self, val): self.val = val
    def to_string(self):
        try:
            val:gdb.Value = self.val
            s = gdb.execute(f"call *to_string(*(amu::BC*){val.address})", to_string = True)
            return s[s.find('=')+2:-1]
        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
pp.add_printer("BC", r"^amu::BC$", BC_printer)

class Member_printer: 
    def __init__(self, val): self.val = val
    def to_string(self):
        try:
            val:gdb.Value = self.val
            s = gdb.execute(f"call *((amu::Member*){val.address})->dump()", to_string = True)
            return s[s.find('=')+2:-1]
        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
pp.add_printer("Member", r"^amu::Member$", Member_printer)

class Type_printer: 
    def __init__(self, val): self.val = val
    def to_string(self):
        try:
            val:gdb.Value = self.val
            s = gdb.execute(f"call *((amu::Type*){val.address})->dump()", to_string = True)
            return s[s.find('=')+2:-1]
        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
pp.add_printer("Type", r"^amu::Type$", Type_printer)

class Var_printer: 
    def __init__(self, val): self.val = val
    def to_string(self):
        try:
            val:gdb.Value = self.val
            s = gdb.execute(f"call *((amu::Var*){val.address})->dump()", to_string = True)
            return s[s.find('=')+2:-1]
        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
pp.add_printer("Var", r"^amu::Var$", Var_printer)

gdb.printing.register_pretty_printer(gdb.current_objfile(), pp)



# commands



import graphviz
import traceback

class graph_ast(gdb.Command):
    def __init__(self):
        super(graph_ast, self).__init__("gast", gdb.COMMAND_USER, gdb.COMPLETE_EXPRESSION)
    
    def build_tree(self, node:gdb.Value):
        print(f"building {node.dereference()}")
        label = ""
        match str(node['kind']):
            case "amu::node::label":
                label = str(node.cast(gdb.lookup_symbol("amu::Label")[0].type.pointer()).dereference())
            case "amu::node::entity":
                label = str(node.cast(gdb.lookup_symbol("amu::Entity")[0].type.pointer()).dereference())
            case "amu::node::statement":
                label = str(node.cast(gdb.lookup_symbol("amu::Statement")[0].type.pointer()).dereference())
            case "amu::node::expression":
                label = str(node.cast(gdb.lookup_symbol("amu::Expression")[0].type.pointer()).dereference())
            case "amu::node::tuple":
                label = str(node.cast(gdb.lookup_symbol("amu::Tuple")[0].type.pointer()).dereference())
            case "amu::node::module":
                label = str(node.cast(gdb.lookup_symbol("amu::Module")[0].type.pointer()).dereference())
            case "amu::node::function":
                label = str(node.cast(gdb.lookup_symbol("amu::Function")[0].type.pointer()).dereference())
            case _:
                print(f"unmatched type: {str(node['kind'])}")
        label = label.split(' ')[0]
        label += f"\n{node['start'].dereference()}\n{node['end'].dereference()}"
        self.dot.node(str(node), label)
        
        current = node['first_child']
        while int(current):
            self.build_tree(current)
            self.dot.edge(str(node), str(current))
            current = current['next']

    def invoke(self, arg, tty):
        try:
            self.dot = graphviz.Digraph()
            self.dot.attr('graph', bgcolor='black', splines='true', concentrate='true')
            self.dot.attr('edge', color='white', arrowhead="none", penwidth='0.5', constraint='true')
            self.dot.attr('node', shape='box', fontcolor='white', color='white', margins='0.08', height='0', width='0', group='0')
            val = gdb.parse_and_eval(arg)
            if val == None:
                print("err")
                return
            if not is_tnode(val):
                print(f"gast requires a TNode or TNode* as its argument")
                return
            
            self.build_tree(val)
            self.dot.render('temp/ast')
        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
            #try to output whatever we got anyways 
            self.dot.render('temp/ast')
graph_ast()

# class graph_ast_stack(gdb.Command):
#     def __init__(self):
#         super(graph_ast_stack, self).__init__("gasts", gdb.COMMAND_USER, gdb.COMPLETE_EXPRESSION)
    
#     def invoke(self, args, tty):
#         stack = gdb.parse_and_eval("amu::parser::internal::stack")
#         if stack == None:
#             print("unable to acquire parser stack")
#             return
#         for i in range(stack['count']):
#             elem = gdb.parse_and_eval()
# graph_ast_stack()

class print_ast(gdb.Command):
    def __init__(self):
        super(print_ast, self).__init__("past", gdb.COMMAND_USER, gdb.COMPLETE_EXPRESSION)

    def invoke(self, arg, tty):
        try: 
            val = gdb.parse_and_eval(arg)
            out = gdb.execute(f"call *(((ASTNode*){val.dereference().address})->print_tree(true))", to_string=True)
            # String's printer turns newlines into '\n' but we want them here, so we need to replace them (again)
            out = out[out.find('=')+2:-1]
            print(out.replace("\\n", "\n"))
        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
print_ast()

class parser_print_stack(gdb.Command):
    def __init__(self):
        super(parser_print_stack, self).__init__("pstack", gdb.COMMAND_USER)
    
    def invoke(self, args, tty):
        try:
            s = gdb.execute("call *display_stack()", to_string=True)
            
            if tty:
                print(s[s.find('=')+2:-1].replace("\\n", "\n"))
            else:
                return s[s.find('=')+2:-1].replace("\\n", "\n")
        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
parser_print_stack()

class display_line(gdb.Command):
    def __init__(self):
        super(display_line, self).__init__("dline", gdb.COMMAND_USER)
    
    def invoke(self, args, tty):
        try: 
            s = gdb.execute("call *token.display_line()", to_string=True)
            if tty:
                print(s[s.find('=')+2:-1].replace("\\n", "\n"))
            else:
                return s[s.find('=')+2:-1].replace("\\n", "\n")
        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
            return 
display_line()

class parser_track_stack(gdb.Command):
    def __init__(self):
        super(parser_track_stack, self).__init__("tstack", gdb.COMMAND_USER)
    
    def invoke(self, args, tty):
        bps = gdb.rbreak(r'^amu::Parser::*')
        for b in bps:
            if 'display_stack' in b.location:
                b.delete()
        # gdb.rbreak(r'amu::Parser::*')
        gdb.execute("r")
        pps = parser_print_stack()
        dline = display_line()
        out = open("temp/track_stack", "w")

        while gdb.selected_inferior().connection_num != None:
            try:
                curfunc = gdb.selected_frame().function()
                lastframe = gdb.selected_frame().older()
                if curfunc == None:
                    gdb.execute("c")
                    continue
                if "push" in curfunc.name:
                    out.write(f"push -------------------------------- {lastframe.older().function()} push\n")
                    gdb.execute("finish")
                elif "pop" in curfunc.name:
                    out.write(f"pop -------------------------------- {lastframe.function()} pop\n")
                    lastframe.select()
                else:
                    gdb.execute("c")
                    continue

                stack = pps.invoke(None, False)
                line = dline.invoke(None, False)

                out.write(stack + "\n" + line + "\n")
                gdb.execute("c")
            except Exception as e:
                print(f"{self.__class__.__name__} error: {e}")
                return 
        gdb.execute("del")
parser_track_stack()
    
class track_locks(gdb.Command):
    def __init__(self):
        super(track_locks, self).__init__("tlocks", gdb.COMMAND_USER)
    
    def invoke(self, args, tty):
        gdb.rbreak("mutex_lock")
        gdb.rbreak("mutex_unlock")
        gdb.rbreak("shared_mutex_lock")
        gdb.rbreak("shared_mutex_unlock")
        
        out = open("temp/track_lock", "w")

        gdb.execute("r")

        while gdb.selected_inferior().connection_num != None:
            try:
                addr = str(gdb.parse_and_eval('m'))
                addr = addr[addr.find("0x"):][:addr.find(" ")]
                cmd = f"info symbol {addr}"
                line = gdb.execute(cmd, to_string=True)[:-1]

                curfunc = gdb.selected_frame().function()
                if "unlock" in curfunc.name:
                    line += " unlock " + str(gdb.selected_thread().global_num) + "\n"
                else:
                    line += " lock " + str(gdb.selected_thread().global_num) + "\n"

                out.write(line)
                
                gdb.execute("c")
            except Exception as e:
                print(f"{self.__class__.__name__} error: {e}")
                return 
track_locks()

import time

class trace_parser(gdb.Command):
    def __init__(self):
        super(trace_parser, self).__init__("tparse", gdb.COMMAND_USER)
    
    def invoke(self, args, tty):
        gdb.rbreak("parser::internal::*")

        out = open("temp/trace_parser", "w")

        gdb.execute("r")

        while gdb.selected_inferior().connection_num != None:
            try:
                curtime = time.perf_counter()
                threadnum = gdb.selected_thread().global_num
                curfunc = gdb.selected_frame().function()

                out.write(f"{curtime}:  {threadnum}  :{curfunc.name}\n")
                gdb.execute("c")

            except Exception as e:
                print(f"{self.__class__.__name__} error: {e}")
                return 
trace_parser()

class emit_gast(gdb.Command):
    def __init__(self):
        super(emit_gast, self).__init__("egast", gdb.COMMAND_USER)
    
    def invoke(self, args, tty):
        try: 
            b = gdb.rbreak("parser::internal::start")[0]
            gdb.execute("r")
            gdb.execute("finish")
            gdb.execute("gast internal::stack.data[0]")
            gdb.execute("c")
            b.delete()
        except Exception as e:
            print(f"{self.__class__.__name__} error: {e}")
            return 
emit_gast()


