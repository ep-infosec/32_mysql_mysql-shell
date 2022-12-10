function validate(expected_prompt, expected_answer) {
    let output = testutil.fetchCapturedStdout(false);
    let lines = output.split('\n')
    let found_prompt = false;
    for (index in lines) {
        line = lines[index].trim()
        if (line.length > 0) {
            let json_doc = JSON.parse(line)
            if (!found_prompt) {
                if (json_doc.hasOwnProperty("prompt")) {
                    found_prompt = true;
                    EXPECT_EQ(expected_prompt, json_doc)
                }
            } else {
                EXPECT_EQ(expected_answer, json_doc)
                break;
            }
        }
    }
    WIPE_OUTPUT()
}

//@<> Implicit TEXT prompt format
// WL14872-TSFR_9_1 All this file is about testing the produced JSON for the different prompts
// WL14872-TSFR_2_1
testutil.callMysqlsh(["-ifull", "--json=raw", "-e","shell.prompt('Implicit TEXT prompt')"], "Answer")
validate({
    prompt:"Implicit TEXT prompt", 
    type:"text"
}, {
    value:"Answer"
})

//@<> Text prompt types with additional options
// WL14872-TSFR_2_1
let text_prompt_types=['text','fileSave', 'fileOpen', 'directory']

for(index in text_prompt_types) {
    WIPE_OUTPUT()
    testutil.callMysqlsh(["-ifull", "--json=raw", "-e",`shell.prompt('Custom Type Prompt', {type:'${text_prompt_types[index]}',defaultValue:'DefaultAnswer', title:'Prompt Title', description:['First Paragraph.', 'Second Paragraph']})`], "Answer")
    validate({
                prompt:"Custom Type Prompt",
                defaultValue:"DefaultAnswer",
                type:`${text_prompt_types[index]}`,
                title:"Prompt Title",
                description:["First Paragraph.","Second Paragraph"]
            }, {
                value: "Answer"
            })
}

//@<> Confirm prompt with default options
// WL14872-TSFR_4_1
// For better UI integration the JSON representation by default includes the yes and no entries
// For backward compatibilities, defaultValue is set to the NO label
WIPE_OUTPUT()
testutil.callMysqlsh(["-ifull", "--json=raw", "-e",`shell.prompt('Confirm Prompt', {type:'confirm'})`], "N")
validate({
    prompt:"Confirm Prompt",
    no:"&No",
    yes:"&Yes",
    type:"confirm"
}, {
    value:"&No"
})

//@<> Confirm prompt with additional options
// WL14872-TSFR_2_1, WL14872-TSFR_4_2
WIPE_OUTPUT()
testutil.callMysqlsh(["-ifull", "--json=raw", "-e",`shell.prompt('Select Replication Method', {type:'confirm', yes:'&Clone', no:'&Incremental', alt:'&Abort', defaultValue:'a', title:'Confirm Title', description:['First Paragraph.', 'Second Paragraph']})`], "a")
validate({
    prompt:"Select Replication Method",
    defaultValue:"&Abort",
    alt:"&Abort",
    no:"&Incremental",
    yes:"&Clone",
    type:"confirm",
    title:"Confirm Title",
    description:["First Paragraph.","Second Paragraph"]
}, {
    value:"&Abort"
})


//@<> Select prompt with default options
WIPE_OUTPUT()
testutil.callMysqlsh(["-ifull", "--json=raw", "-e",`shell.prompt('Select Option', {type:'select', options:['One','Two']})`], "1")
validate({
    prompt:"Select Option",
    type:"select",
    options:["One","Two"]
}, {
    value:"One"
})


//@<> Select prompt with additional options
// WL14872-TSFR_2_1
WIPE_OUTPUT()
testutil.callMysqlsh(["-ifull", "--json=raw", "-e",`shell.prompt('Select Option', {type:'select', options:['One','Two'], defaultValue:2, title:'Select Title', description:['First Paragraph.', 'Second Paragraph']})`], "1")
validate({
    prompt:"Select Option",
    type:"select",
    title:"Select Title",
    description:["First Paragraph.","Second Paragraph"],
    defaultValue:2,
    options:["One","Two"]
}, {
    value:"One"
})
