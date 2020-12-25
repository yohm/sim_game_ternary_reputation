lines = File.open(ARGV[0]).map(&:to_i)
n = ARGV[1].to_i
d = lines.size / n
ans = []
n.times do |i|
  ans << lines[d*i]
end

puts ans

