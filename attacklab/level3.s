	.globl	main
main:
	movl	$0x5561dc74, %edi
	movabsq	$7018357788218898741, %rdx
	movq	%rdx, (%rdi)
	movb	$0, 8(%rdi)
	pushq	$0x4018fa
	ret
