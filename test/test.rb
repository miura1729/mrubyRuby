require 'mruby'

#MRuby.eval('p 1')

MRuby.eval('def foo(x)
              if x < 1 then 1 
              else 
                foo(x - 1) * x 
              end
            end
            p foo(5)')

