.global main
main:

# TODO: [p0-task1] Add your code here!
	li 	t0, 0
	li 	t1, 1
	li 	t2, 100
loop: 
	add 	t0, t0, t1
	addi 	t1, t1, 1
	ble 	t1, t2, loop 
end:
	j end
