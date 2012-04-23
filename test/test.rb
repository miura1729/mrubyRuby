require 'mruby'

#MRuby.eval('p 1')

p MRuby.eval('def foo(x)
              if x < 1 then 1 
              else 
                foo(x - 1) * x 
              end
            end
            p foo(5)')


p MRuby.eval('"foo"')
p MRuby.eval('1.9')
p MRuby.eval('[1, 2, 3]')

a = MRuby.new
p a
p a.gen_code("p 1").to_a

