CC=gcc
CCFLAGS=-g
FILES=main.c boot_loader.c class_printer.c class.c instance.c array.c object.c interpreter.c cli.c jvm.c
BUILD=build
SOURCES = $(foreach file,$(FILES),src/c/$(file))
EXECUTABLE=build/vm

JAVA_BUILD_STD=build/std
JAVA_STD_CLASSES=BuiltinPrinter PrintStream System Object String
JAVA_STD_SOURCES = $(foreach file,$(JAVA_STD_CLASSES),src/java/std/$(file).java)
JAVA_STD_CLASS_FILES = $(foreach file,$(JAVA_STD_CLASSES),$(JAVA_BUILD_STD)/$(file).class)

all: $(EXECUTABLE)

$(EXECUTABLE): $(SOURCES)
	@$(CC) $(CCFLAGS) -o $(EXECUTABLE) $(SOURCES)

stdlib: $(JAVA_STD_CLASS_FILES)

$(JAVA_STD_CLASS_FILES): $(JAVA_STD_SOURCES)
	@javac -d $(JAVA_BUILD_STD) -h $(JAVA_BUILD_STD) -s $(JAVA_BUILD_STD) -bootclasspath src/java/std/rt $(JAVA_STD_SOURCES)

build/Test1.class: src/java/Test1.java
	@javac -d $(BUILD) -h $(BUILD) -s $(BUILD) -cp build/std src/java/Test1.java

build/Test2.class: src/java/Test2.java
	@javac -d $(BUILD) -h $(BUILD) -s $(BUILD) src/java/Test2.java

build/TestNative.class: src/java/TestNative.java
	@javac -d $(BUILD) -h $(BUILD) -s $(BUILD) src/java/TestNative.java

Test1: build/Test1.class $(EXECUTABLE)
	@$(EXECUTABLE) -cp build Test1

Test2: build/Test2.class $(EXECUTABLE)
	@$(EXECUTABLE) -cp build Test2

TestNative: build/TestNative.class $(EXECUTABLE)
	@$(EXECUTABLE) -cp build TestNative

clean:
	@rm -rf build/*
